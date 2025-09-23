
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h> 
#include <threads.h>
#include <limits.h>

#include "util/data_structure/dynamic_string.h"
#include "util/system.h"

#include "logger.h"


#define USE_MULTI_THREADING     1


// ============================================================================================================================================
// format helper
// ============================================================================================================================================

inline static const char *short_filename(const char *path) {

    const char *s1 = strrchr(path, '/');
    const char *s2 = strrchr(path, '\\');
    const char *last = NULL;

    if (s1 && s2) 
        last = (s1 > s2) ? s1 : s2;
    else
        last = s1 ? s1 : s2;

    return last ? last + 1 : path;
}


static const char* c_severity_names[] =       { "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL" };


const char* log_level_to_string(log_type t) {

    if ((int)t < 0 || (size_t)t >= sizeof(c_severity_names) / sizeof(c_severity_names[0]))
        return "UNK";
    return c_severity_names[(int)t];
}


static const char* c_console_color_table[] = {
    "\033[90m",                 // TRACE - gray
    "\033[94m",                 // DEBUG - blue
    "\033[92m",                 // INFO  - green
    "\033[93m",                 // WARN  - yellow
    "\033[91m",                 // ERROR - red
    "\x1B[1m\x1B[37m\x1B[41m"   // FATAL - bold white on red
};

static const char*              c_console_rest = "\033[0m";

static const char*              c_default_format = "[$B$T $L] $E $P:$G $C$Z";

static char*                    s_format_current = NULL;


// ============================================================================================================================================
// thread label map (simple linked list)
// ============================================================================================================================================

typedef struct thread_label_node {
    u64                         thread_id;
    char*                       label;
    struct thread_label_node*   next;
} thread_label_node;

static thread_label_node*       s_thread_labels = NULL;

static pthread_mutex_t          s_general_mutex = PTHREAD_MUTEX_INITIALIZER;


void logger_register_thread_label(pthread_t thread_id, const char* label) {

    // TODO: only print this to the log file
    // printf("registering thread [%ul] under [%s]\n", thread_id, label);

    pthread_mutex_lock(&s_general_mutex);
    struct thread_label_node* current = s_thread_labels;
    while (current) {
        if (current->thread_id == thread_id) {
            free(current->label);
            current->label = strdup(label ? label : "");
            pthread_mutex_unlock(&s_general_mutex);
            return;
        }
        current = current->next;
    }
    // not found, append
    struct thread_label_node* node = malloc(sizeof(*node));
    node->thread_id = thread_id;
    node->label = strdup(label ? label : "");
    node->next = s_thread_labels;
    s_thread_labels = node;
    pthread_mutex_unlock(&s_general_mutex);
}


const char* lookup_thread_label(pthread_t thread_id) {

    struct thread_label_node* n = s_thread_labels;
    while (n) {
        if (n->thread_id == thread_id) {

            pthread_mutex_unlock(&s_general_mutex);
            return n->label;
        }
        n = n->next;
    }
    return NULL;
}


void logger_remove_thread_label_by_id(pthread_t thread_id) {
    
    pthread_mutex_lock(&s_general_mutex);
    thread_label_node *current = s_thread_labels;
    thread_label_node *prev = NULL;
    
    while (current) {
        if (current->thread_id == thread_id) {
            // Found the node to remove
            if (prev) {
                prev->next = current->next;
            } else {
                s_thread_labels = current->next;
            }
            
            free(current->label);
            free(current);
            break;
        }
        
        prev = current;
        current = current->next;
    }
    
    pthread_mutex_unlock(&s_general_mutex);
}


void logger_remove_thread_label_by_label(const char* label) {
    
    if (!label) return;
    pthread_mutex_lock(&s_general_mutex);
    
    thread_label_node *current = s_thread_labels;
    thread_label_node *prev = NULL;
    
    while (current) {
        if (strcmp(current->label, label) == 0) {
            // Found the node to remove
            if (prev) {
                prev->next = current->next;
            } else {
                s_thread_labels = current->next;
            }
            
            free(current->label);
            free(current);
            break;
        }
        
        prev = current;
        current = current->next;
    }
    
    pthread_mutex_unlock(&s_general_mutex);
}


void logger_remove_all_thread_labels() {
    
    pthread_mutex_lock(&s_general_mutex);
    thread_label_node *current = s_thread_labels;
    while (current) {
        thread_label_node *next = current->next;
        free(current->label);
        free(current);
        current = next;
    }
    
    s_thread_labels = NULL;
    pthread_mutex_unlock(&s_general_mutex);
}


// ============================================================================================================================================
// multi threading
// ============================================================================================================================================

#define STR_LEN 512                         // maximum string length (used for file/function name)
#define MSG_LEN 32000                       // maximum log message length (should never be needed, but ...)

#if USE_MULTI_THREADING

    static pthread_t s_logger_thread;           // logger thread
    

    typedef struct {
        
        log_type    type;
        pthread_t   thread_id;                  // as provided by (u64)pthread_self()
        char        file_name[STR_LEN];         // as provided by __FILE__
        char        function_name[STR_LEN];     // as provided by __FUNCTION__
        int         line;                       // as provided by __LINE__
        char        message[MSG_LEN];           // already formatted to contain the values of the variable at the time the user called LOG()
    } log_msg;

    // Circular buffer for log_msg
    typedef struct {

        log_msg *items;                         // array of items
        size_t capacity;                        // max number of items
        size_t head;                            // index of oldest item
        size_t tail;                            // index to write next item
        size_t count;                           // current number of items
        pthread_mutex_t mutex;
        pthread_cond_t not_full;                // signal producers when space available
        pthread_cond_t contains;                // signal logger that messages are ready for processing
        int shutdown;                           // set to 1 to tell threads to quit
    } log_msg_buffer;

    static log_msg_buffer   s_log_msg_buffer;


    // Initialize buffer
    int buffer_init(log_msg_buffer* b, size_t capacity) {
        
        if (!b || capacity == 0)return EINVAL;

        b->items = calloc(capacity, sizeof(log_msg));
        if (!b->items) return ENOMEM;

        b->capacity = capacity;
        b->head = b->tail = b->count = 0;
        b->shutdown = 0;

        // try to init the pthread vars
        if (pthread_mutex_init(&b->mutex, NULL) != 0) { 
            free(b->items);
            return -1;
        }
        
        if (pthread_cond_init(&b->not_full, NULL) != 0) { 
            pthread_mutex_destroy(&b->mutex); 
            free(b->items); 
            return -1;
        }

        if (pthread_cond_init(&b->contains, NULL) != 0) { 
            pthread_cond_destroy(&b->not_full); 
            pthread_mutex_destroy(&b->mutex); 
            free(b->items); 
            return -1;
        }
        return 0;
    }

    // Destroy buffer
    void buffer_destroy(log_msg_buffer* b) {
        
        if (!b) return;
        pthread_cond_destroy(&b->contains);
        pthread_cond_destroy(&b->not_full);
        pthread_mutex_destroy(&b->mutex);
        free(b->items);
    }

    // Producer pushes an item; blocks if buffer is full. Returns 0 on success, -1 on shutdown.
    int buffer_push(log_msg_buffer* b, const log_msg* item) {

        int err = 0;
        pthread_mutex_lock(&b->mutex);
        while (b->count == b->capacity && !b->shutdown) {
            pthread_cond_wait(&b->not_full, &b->mutex); // wait until space
        }
        if (b->shutdown) {
            err = -1;
            goto out;
        }
        // copy item into buffer at tail
        b->items[b->tail] = *item; // struct copy (safe because str is fixed-length array)
        b->tail = (b->tail + 1) % b->capacity;
        b->count++;

        pthread_cond_signal(&b->contains);  // notify logger that a new message is present
    out:
        pthread_mutex_unlock(&b->mutex);
        return err;
    }

    // Consumer pops an item; blocks if buffer is empty. Returns 0 on success, -1 on shutdown.
    static int buffer_pop(log_msg_buffer* b, log_msg* out) {

        int err = 0;
        pthread_mutex_lock(&b->mutex);
        while (b->count == 0 && !b->shutdown)
            pthread_cond_wait(&b->contains, &b->mutex);     // wait until items available
        
        if (b->shutdown && b->count == 0) {
            err = -1;
            goto out;
        }
        // copy item from buffer at head
        *out = b->items[b->head]; // struct copy
        b->head = (b->head + 1) % b->capacity;
        b->count--;
        
        pthread_cond_signal(&b->not_full); // notify producers that space is available
    out:
        pthread_mutex_unlock(&b->mutex);
        return err;
    }


    void process_log_message_v(const log_msg* message);

#else

    void process_log_message_v(log_type type, pthread_t thread_id, const char* file_name, const char* function_name, int line, const char* formatted_message);

#endif


// ============================================================================================================================================
// data
// ============================================================================================================================================

static b8                       s_log_to_console = false;

static char*                    s_log_file_path = NULL;

#define FILE_BUFFER_SIZE        64000

static char                     s_file_buffer[FILE_BUFFER_SIZE] = {0};          // buffer fully formatted messages here before writing to file

static pthread_mutex_t          s_file_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;


// ============================================================================================================================================
// private functions
// ============================================================================================================================================

// CAUTION: !!! need to manually manage the mutex [s_file_buffer_mutex] !!!
// normal one would add the mutex protection in the function, but this function is only used in two location and both have spatial circumstances
bool flush_log_msg_buffer(const char* log_msg) {
    
    if (s_log_file_path == NULL)
        return false;


    FILE* fp = fopen(s_log_file_path, "a");
    if (fp == NULL)
        BREAK_POINT();          // logger failed to create log file

    // TODO: dump content of buffer
    fprintf(fp, "%s", s_file_buffer);
    memset(s_file_buffer, '\0', sizeof(s_file_buffer));

    if (log_msg)                            // log provided message if exist
        fprintf(fp, "%s", log_msg);

    fclose(fp);
    return true;
}


#if USE_MULTI_THREADING           // give message to buffer and let logger-thread perform processing

    // thread function that waits for messages in the s_lg_msg_buffer and then processes them using the multithread version of [process_log_message_v]
    static void* logger_thread_func(void* arg) {
        (void)arg;
        
        while (1) {
            log_msg msg;
            if (buffer_pop(&s_log_msg_buffer, &msg) == -1) {
                break; // shutdown requested
            }
            
            // Process the message
            process_log_message_v(&msg);
        }
        
        return NULL;
    }

#endif


// ============================================================================================================================================
// LOGGER
// ============================================================================================================================================


b8 logger_init(const char* log_msg_format, const b8 log_to_console, const char* log_dir, const char* log_file_name, const b8 use_append_mode) {

    s_log_to_console = log_to_console;
    logger_set_format(log_msg_format);

    const char* exec_path = get_executable_path();
    if (exec_path == NULL) return false;

    char file_path[PATH_MAX];
    memset(file_path, '\0', sizeof(file_path));
    snprintf(file_path, sizeof(file_path), "%s/%s", exec_path, log_dir);
    // printf("log path: %s\n", file_path);

    if (mkdir(file_path, 0777) && errno != EEXIST)
        BREAK_POINT();

    memset(file_path, '\0', sizeof(file_path));
    snprintf(file_path, sizeof(file_path), "%s/%s/%s.log", exec_path, log_dir, log_file_name);
    s_log_file_path = strdup(file_path);
    ASSERT_SS(s_log_file_path)

    system_time st = get_system_time();
    FILE* fp = fopen(s_log_file_path, (use_append_mode) ? "a" : "w");
    ASSERT_SS(fp)

    fprintf(fp, "=====================================================================================================\n");
    fprintf(fp, "Log initalized at [%04d/%02d/%02d %02d:%02d:%02d] with format: %s\n", st.year, st.month, st.day, st.hour, st.minute, st.second, s_format_current);
    fprintf(fp, "-----------------------------------------------------------------------------------------------------\n");

    fclose(fp);

#if USE_MULTI_THREADING

    ASSERT_SS(!buffer_init(&s_log_msg_buffer, 32));          // use 32 for now
    ASSERT_SS(pthread_create(&s_logger_thread, NULL, logger_thread_func, NULL) == 0);

#endif

    return true;
}


b8 logger_shutdown() {

#if USE_MULTI_THREADING
    /* Signal shutdown to all waiters (consumers & producers) */
    pthread_mutex_lock(&s_log_msg_buffer.mutex);
    s_log_msg_buffer.shutdown = 1;
    pthread_cond_broadcast(&s_log_msg_buffer.contains);
    pthread_cond_broadcast(&s_log_msg_buffer.not_full);
    pthread_mutex_unlock(&s_log_msg_buffer.mutex);

    /* Don't try to join ourselves (would deadlock). If thread wasn't created, skip. */
    if (!pthread_equal(pthread_self(), s_logger_thread)) {
        pthread_join(s_logger_thread, NULL);
    }

    buffer_destroy(&s_log_msg_buffer);
    logger_remove_all_thread_labels();
#endif


    system_time st = get_system_time();
    dyn_str out;
    ds_init(&out);

    ds_append_str(&out, "-----------------------------------------------------------------------------------------------------\n");
    ds_append_fmt(&out, NULL, "Closing Log at [%04d/%02d/%02d %02d:%02d:%02d]\n", st.year, st.month, st.day, st.hour, st.minute, st.second);
    ds_append_str(&out, "=====================================================================================================\n");

    // mutex management not needed as the only other thread working on the same buffer joint just befor
    ASSERT_SS(flush_log_msg_buffer(out.data))

    ds_free(&out);

    // Free allocated resources
    free(s_log_file_path);
    free(s_format_current);
    s_log_file_path = NULL;
    s_format_current = NULL;

    return true;
}


void logger_set_format(const char* new_format) {

    pthread_mutex_lock(&s_general_mutex);
    free(s_format_current);
    if (new_format)
        s_format_current = strdup(new_format);
    else
        s_format_current = strdup(c_default_format);
        
    ASSERT(s_format_current, "", "something went wrong when duplicating the string")
    pthread_mutex_unlock(&s_general_mutex);
}


// ============================================================================================================================================
// message formatter
// ============================================================================================================================================

#if USE_MULTI_THREADING           // give message to buffer and let logger-thread perform processing

    // for multithreading that uses a [log_msg] as param
    void process_log_message_v(const log_msg* message) {
        
        if (strlen(message->message) == 0)      // skip empty messages
            return;

        system_time st = get_system_time();

        // build formatted output according to s_format_current
        pthread_mutex_lock(&s_general_mutex);
        const char* fmt = s_format_current ? s_format_current : c_default_format;

        dyn_str out;
        ds_init(&out);

        size_t fmt_len = strlen(fmt);
        for (size_t i = 0; i < fmt_len; ++i) {
            char c = fmt[i];
            if (c == '$') {
                if (i + 1 >= fmt_len) break;
                char cmd = fmt[++i];
                switch (cmd) {
                    case 'B': ds_append_str(&out, c_console_color_table[(int)message->type]); break;                    // color begin
                    case 'E': ds_append_str(&out, c_console_rest); break;                                               // color end
                    case 'C': ds_append_str(&out, message->message); break;                                             // message content
                    case 'L': ds_append_str(&out, log_level_to_string(message->type)); break;                           // severity
                    case 'Z': ds_append_char(&out, '\n'); break;                                                        // newline
                    case 'Q': {                                                                                         // thread id or label
                        const char* label = lookup_thread_label(message->thread_id);
                        if (label)  ds_append_str(&out, label);
                        else        ds_append_fmt(&out, "%lu", message->thread_id);
                    } break;
                    case 'F': ds_append_str(&out, message->function_name); break;                                       // function
                    case 'A': ds_append_str(&out, message->file_name); break;                                           // file
                    case 'I': ds_append_str(&out, short_filename(message->file_name)); break;                           // short file
                    case 'G': ds_append_fmt(&out, NULL, "%d", message->line); break;                                    // line
                    
                    case 'T': ds_append_fmt(&out, NULL, "%02d:%02d:%02d", st.hour, st.minute, st.second); break;        // time component
                    case 'H': ds_append_fmt(&out, NULL, "%02d", st.hour); break;                                        // time component
                    case 'M': ds_append_fmt(&out, NULL, "%02d", st.minute); break;                                      // time component
                    case 'S': ds_append_fmt(&out, NULL, "%02d", st.second); break;                                      // time component
                    case 'J': ds_append_fmt(&out, NULL, "%03d", st.millisec); break;                                    // time component

                    case 'N': ds_append_fmt(&out, NULL, "%04d/%02d/%02d", st.year, st.month, st.day); break;            // date component
                    case 'Y': ds_append_fmt(&out, NULL, "%04d", st.year); break;                                        // date component
                    case 'O': ds_append_fmt(&out, NULL, "%02d", st.month); break;                                       // date component
                    case 'D': ds_append_fmt(&out, NULL, "%02d", st.day); break;                                         // date component

                    default:                                                                                            // unknown %% - treat literally (append '$' and the char)
                        ds_append_char(&out, '$');
                        ds_append_char(&out, cmd);
                        break;
                }
            } else {
                ds_append_char(&out, c);
            }
        }

        // ensure final message ends with newline
        if (out.len == 0 || out.data[out.len - 1] != '\n') ds_append_char(&out, '\n');


        // route to stdout or stderr depending on severity
        if (s_log_to_console) {
            if ((int)message->type < LOG_TYPE_WARN) {
                fputs(out.data, stdout);
                fflush(stdout);
            } else {
                fputs(out.data, stderr);
                fflush(stderr);
            }
        }


        const size_t msg_length = strlen(out.data);
        
        pthread_mutex_lock(&s_file_buffer_mutex);       // use mutex outside here because of strlen()
        const size_t remaining_buffer_size = sizeof(s_file_buffer) - strlen(s_file_buffer) -1;
        if (remaining_buffer_size > msg_length)
            strcat(s_file_buffer, out.data);              // save because ensured size
        else
            flush_log_msg_buffer(out.data);      // flush all buffered messages and current message
        pthread_mutex_unlock(&s_file_buffer_mutex);


        ds_free(&out);
        pthread_mutex_unlock(&s_general_mutex);
    }

#else

    // ----------------- main formatter - expects the message text to be already formatted -----------------
    void process_log_message_v(log_type type, pthread_t thread_id, const char* file_name, const char* function_name, int line, const char* formatted_message) {
        
        if (!formatted_message)
            formatted_message = "";

        system_time st = get_system_time();

        // build formatted output according to s_format_current
        pthread_mutex_lock(&s_general_mutex);
        const char* fmt = s_format_current ? s_format_current : c_default_format;

        dyn_str out;
        ds_init(&out);

        size_t fmt_len = strlen(fmt);
        for (size_t i = 0; i < fmt_len; ++i) {
            char c = fmt[i];
            if (c == '$') {
                if (i + 1 >= fmt_len) break;
                char cmd = fmt[++i];
                switch (cmd) {
                    case 'B': ds_append_str(&out, c_console_color_table[(int)type]); break;                           // color begin
                    case 'E': ds_append_str(&out, c_console_rest); break;                                             // color end
                    case 'C': ds_append_str(&out, formatted_message); break;                                        // message content
                    case 'L': ds_append_str(&out, log_level_to_string(type)); break;                                // severity
                    case 'Z': ds_append_char(&out, '\n'); break;                                                    // newline
                    case 'Q': {                                                                                     // thread id or label
                        const char* label = lookup_thread_label(thread_id);
                        if (label)  ds_append_str(&out, label);
                        else        ds_append_fmt(&out, "%lu", thread_id);
                    } break;
                    case 'F': ds_append_str(&out, function_name ? function_name : ""); break;                       // function
                    case 'P': ds_append_str(&out, function_name); break;                                            // short function
                    case 'A': ds_append_str(&out, file_name ? file_name : ""); break;                               // file
                    case 'I': ds_append_str(&out, short_filename(file_name ? file_name : "")); break;               // short file
                    case 'G': ds_append_fmt(&out, "%d", line); break;                                               // line
                    
                    case 'T': ds_append_fmt(&out, "%02d:%02d:%02d", st.hour, st.minute, st.second); break;          // time component
                    case 'H': ds_append_fmt(&out, "%02d", st.hour); break;                                          // time component
                    case 'M': ds_append_fmt(&out, "%02d", st.minute); break;                                        // time component
                    case 'S': ds_append_fmt(&out, "%02d", st.second); break;                                        // time component
                    case 'J': ds_append_fmt(&out, "%03d", st.millisec); break;                                      // time component

                    case 'N': ds_append_fmt(&out, "%04d/%02d/%02d", st.year, st.month, st.day); break;              // date component
                    case 'Y': ds_append_fmt(&out, "%04d", st.year); break;                                          // date component
                    case 'O': ds_append_fmt(&out, "%02d", st.month); break;                                         // date component
                    case 'D': ds_append_fmt(&out, "%02d", st.day); break;                                           // date component

                    default:                                                                                        // unknown %% - treat literally (append '$' and the char)
                        ds_append_char(&out, '$');
                        ds_append_char(&out, cmd);
                        break;
                }
            } else {
                ds_append_char(&out, c);
            }
        }

        // ensure final message ends with newline
        if (out.len == 0 || out.data[out.len - 1] != '\n') ds_append_char(&out, '\n');


        // route to stdout or stderr depending on severity
        if (s_log_to_console) {
            if ((int)type < LOG_TYPE_WARN) {
                fputs(out.data, stdout);
                fflush(stdout);
            } else {
                fputs(out.data, stderr);
                fflush(stderr);
            }
        }


        const size_t msg_length = strlen(out.data);
        const size_t remaining_buffer_size = sizeof(s_file_buffer) - strlen(s_file_buffer) -1;
        if (remaining_buffer_size > msg_length)
            strcat(s_file_buffer, out.data);              // save because ensured size
        else
            flush_log_msg_buffer(out.data);      // flush all buffered messages and current message


        ds_free(&out);
        pthread_mutex_unlock(&s_general_mutex);
    }

#endif


void log_message(log_type type, pthread_t thread_id, const char* file_name, const char* function_name, const int line, const char* message, ...) {

    if (strlen(message) == 0)
        return;                                             // skip all empty log messages
    
    // use fixed size stack buffer (this forces a max log message length, but much faster than dynamic heap allocation)
    char loc_message[MSG_LEN];
    memset(&loc_message, 0, sizeof(loc_message));

    // Format the user message first (variable args)
    va_list ap;
    va_start(ap, message);
    vsnprintf(loc_message, sizeof(loc_message), message, ap);
    va_end(ap);

#if USE_MULTI_THREADING                                     // give message to buffer and let logger-thread perform processing

    log_msg current_msg;
    memset(&current_msg, 0, sizeof(current_msg));           // Initialize to zero

    current_msg.type = type;
    current_msg.thread_id = thread_id;
    current_msg.line = line;
    // Use strncpy to prevent buffer overflows
    strncpy(current_msg.file_name, file_name, sizeof(current_msg.file_name) - 1);
    strncpy(current_msg.function_name, function_name, sizeof(current_msg.function_name) - 1);
    strncpy(current_msg.message, loc_message, sizeof(current_msg.message) - 1);

    buffer_push(&s_log_msg_buffer, &current_msg);

#else                                                       // direct processing in calling thread

    process_log_message_v(type, thread_id, file_name, function_name, line, loc_message);        // call the formatter that understands s_format_current

#endif

}
