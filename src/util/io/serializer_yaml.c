
#include <errno.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "util/io/logger.h"
#include "util/util.h"
#include "util/data_structure/data_types.h"
#include "util/system.h"

#include "util/io/serializer_yaml.h"



/*

file buffer for dev:

test_section_00:
  random_00: dfkjvhbuswzvbauzvbaufbvaf
  random_01: skjdfhbvzsuzt fsdh bfvhflk
test_section_00:
  random_00: dfkjvhbuswzvbauzvbaufbvaf
  random_01: skjdfhbvzsuzt fsdh bfvhflk


main_section:
test_i32: 4294919028
test_f32: 0.000000
test_bool: 4294919032
test_long_long: nan
test_str: Since C doesn't support switching on strings directly, we need to use a different approach.

*/


// ============================================================================================================================================
// helper functions
// ============================================================================================================================================

#define STR_LINE_LEN    32000               // !!! longest possible length for a line in YAML file !!!


// will get a char array like his: [char line[STR_LINE_LEN]]
u32 get_indentation(const char *line) {

    if (!line) return 0;
    u32 count = 0;
    u32 pointer = 0;
    
    while ((line[pointer] == ' ' && line[pointer +1] == ' ') || line[count] == '\t') {

        pointer += (line[count] == '\t') ? 1 : 2 ;          // shift pointer by 2 if spaces
        count++;
    }

    return count;
}


// will get a char array like his: [char line[STR_LINE_LEN]]
u32 get_indentation_reverse(const char *line) {

    if (!line) return 0;
    i32 count = -1;
    i32 pointer = -1;
    
    while ((line[pointer] == ' ' && line[pointer -1] == ' ') || line[count] == '\t') {

        pointer -= (line[count] == '\t') ? 1 : 2 ;          // shift pointer by 2 if spaces
        count--;
    }

    return -(count +1);
}


//
const char* skip_indentation(const char* line) {
    
    if (!line) return NULL;

    size_t i = 0;
    while (line[i] == ' ' || line[i] == '\t')
        i++;
    
    return line + i;  // Return pointer past indentation
}


// ============================================================================================================================================
// section handling
// ============================================================================================================================================

// get all lines that match the section and indentation and save them in [serializer->section_content]
// lines inside [serializer->section_content] are "\n" terminated
b8 get_content_of_section(SY* serializer) {

    // reset string
    ds_free(&serializer->section_content);
    ds_init(&serializer->section_content);
    rewind(serializer->fp);

    // search for header while respecting the hierarchy in [serializer->section_headers]
    char line[STR_LINE_LEN] = {0};
    b8 found_section = true;                                // if hierarchy is not violated this will remain true
    const size_t number_of_headers = stack_size(&serializer->section_headers);
    LOG(Trace, "number_of_headers %zu", number_of_headers)
    for (size_t x = 0; x < number_of_headers; x++) {
        
        char current_header[STR_SEC_LEN] = {0};
        stack_peek_at(&serializer->section_headers, x, &current_header);
        LOG(Trace, "searching for [%s]", current_header)

        while (fgets(line, sizeof(line), serializer->fp)) {

            LOG(Trace, "current line [%s]", line)
            const u32 indent = get_indentation(line);
            if (indent < x) {                               // left header hierarchy
                found_section = false;                      // hierarchy violated -> still needs to search for subsections
                goto break_search;
            }

            //  current header                  correct indentation (going deeper in)
            if (strstr(line, current_header) && indent == x)
                break;      // exit search loop -> found header        continue FOR to search for next header
        }
    }
    break_search:
    VALIDATE(found_section, return false, "", "could not find section ")

    // Prepare regex to match key-value lines
    static const char *pattern = "^[ \t]*[A-Za-z0-9_-]+:[ \t]*[^ \t\n]+.*$";
    regex_t regex;
    VALIDATE(!regcomp(&regex, pattern, REG_EXTENDED), return false, "", "Regex compilation failed")
    
    // pars all lines that come after
    while (fgets(line, sizeof(line), serializer->fp)) {

        const u32 indent = get_indentation(line);
        if (indent < serializer->current_indentation) break;         // stop when section ends
        if (indent > serializer->current_indentation) continue;      // skip any potential subsection

        // check if line looks like this using regex:      <leading indentation><string>: <string>
        if (regexec(&regex, line, 0, NULL, 0) == 0)
            ds_append_str(&serializer->section_content, skip_indentation(line));
    }
    regfree(&regex); // Don't forget to free the regex

    char current_header[STR_SEC_LEN] = {0};
    stack_peek(&serializer->section_headers, &current_header);
    LOG(Info, "current_header [%s] serializer->section_content: \n%s", current_header, serializer->section_content.data)

    return true;
}


typedef struct {
    SY*    serializer;
    const char*         start;
    const char*         end;
    dyn_str*            file_content;
    size_t              headers_index;
    b8                  found_last_section;
} serializer_section_data;


b8 find_section_start_and_end_callback(const char* line, size_t len, void* user_data) {

    serializer_section_data* sec_data = (serializer_section_data*)user_data;

    char current_header[STR_SEC_LEN] = {0};
    stack_peek_at(&sec_data->serializer->section_headers, sec_data->headers_index, &current_header);
    // LOG(Trace, "searching for [%s] of [%s] in [%zu][%.*s]", (sec_data->found_last_section) ? "END" : "START", current_header, len, len, line)

    const u32 indent = get_indentation(line);
    const b8 last_section = (stack_size(&sec_data->serializer->section_headers) -1) == sec_data->headers_index;
    if (!sec_data->found_last_section) {                // Find start first: currect section (name and indentation)

        if (indent < sec_data->headers_index) {         // exited header hierarchy
            return false;                               // hierarchy violated -> still needs to search for subsections
        }

        //  check indentation (remove 1 for header)                     correct title
        if (indent == (sec_data->headers_index) && str_search_range(line, current_header, len)) {
            sec_data->start = line + len;               // update start pointer to ref last found section title ()
            
            if (last_section)
                sec_data->found_last_section = true;    // found las header start -> should start searching for the end
            else
                sec_data->headers_index++;              // move on to next header
        }
        
        return true;                                    // always return true because ds_iterate_lines still need to continue until end if found
    } 

    // Find end of section
    if (indent < sec_data->serializer->current_indentation || (line[len -1] == ':' && indent == (sec_data->serializer->current_indentation -1))) {
        sec_data->end = line -1;
        return false;
    }

    return true;
}


b8 add_or_update_entry(const char* line, size_t len, void* user_data) {
    
    serializer_section_data* sec_data = (serializer_section_data*)user_data;
    // const char* unindented_line = skip_indentation(line);
    // LOG(Debug, "line: [%.*s]", (int)len, line)
    
    // Get key
    char key_str[PATH_MAX] = {0};
    size_t key_len = 0;
    for (size_t x = 0; x < len; x++) {
        if (line[x] == ':') {
            
            key_str[x] = line[x];
            break;
        }

        key_str[x] = line[x];
        key_len++;
    }
    // LOG(Trace, "key_str: %s", key_str)

    // const size_t end_position = (sec_data->end - sec_data->file_content->data);                                         // Search for key in the section range
    const char* section_start = sec_data->file_content->data + (sec_data->start - sec_data->file_content->data);        // Search only within the section bounds
    const char* key_location_in_file = str_search_range(section_start, key_str, (sec_data->end - sec_data->file_content->data));
    const u32 key_indent = get_indentation_reverse(key_location_in_file);
    if (!key_location_in_file || key_indent != sec_data->serializer->current_indentation) {       // Key not found, append to end of section

        const int indent_spaces = (sec_data->serializer->current_indentation) * 2;
        char indent_str[64] = {0};
        if (indent_spaces > 0 && indent_spaces < (int)sizeof(indent_str))
            memset(indent_str, ' ', indent_spaces);
        
        char new_line[PATH_MAX];
        snprintf(new_line, sizeof(new_line), "\n%s%.*s", indent_str, (int)len, line);
        
        size_t end_pos = sec_data->end - sec_data->file_content->data;      // Calculate the correct insertion position (need to add 1 for \n)
        if (end_pos > sec_data->file_content->len)                          // Make sure we're not inserting beyond the string length
            end_pos = sec_data->file_content->len;
        
        LOG(Trace, "trying to insert new line [%s] at %zu", new_line, end_pos);
        const i32 result = ds_insert_str(sec_data->file_content, end_pos, new_line);
        if (result != AT_SUCCESS) {
            LOG(Error, "Failed to insert new line [%s] because [%d]", new_line, result);
            return true;
        }
        
        sec_data->end = sec_data->file_content->data + end_pos + strlen(new_line);      // Update end pointer to account for the new content
        return true;
    }

    // Key found, update the value

    // Find the value position in the file
    char* file_value_start = strchr(key_location_in_file, ':');
    if (!file_value_start) return true;
    file_value_start++; // Skip colon
    
    // Skip whitespace
    while (*file_value_start == ' ' || *file_value_start == '\t')
        file_value_start++;
    
    // Find end of current value (newline or end of string)
    char* file_value_end = strchr(file_value_start, '\n');
    if (!file_value_end) file_value_end = sec_data->file_content->data + sec_data->file_content->len;
    
    size_t file_value_pos = file_value_start - sec_data->file_content->data;
    size_t file_value_len = file_value_end - file_value_start;

    

    const char* value_start = strchr(line, ':');
    if (!value_start) return true;
    value_start++;                                                      // Skip colon
    while (*value_start == ' ' || *value_start == '\t') {               // Skip whitespace after colon
        value_start++;
        len--;
    }
    size_t value_len = len - (value_start - line) +1;
    char value_str[PATH_MAX] = {0};
    strncpy(value_str, value_start, value_len);
    
    i32 result = ds_replace_range(sec_data->file_content, file_value_pos, file_value_len, value_str);
    if (result != AT_SUCCESS)
        LOG(Error, "ds_replace_range failed: %d", result)

    return true;
}


void save_section(SY* serializer) {

    // load entire file content into dyn_str
    dyn_str file_content = {0};
    const i32 result = ds_from_file(&file_content, serializer->fp);
    VALIDATE(!result, return, "", "Error reading file: %d", result)

    LOG(Info, "file_content: \n%s\n", file_content.data)
    LOG(Info, "section_content: \n%s\n", serializer->section_content.data)

    serializer_section_data sec_data = {0};
    sec_data.serializer = serializer;
    ds_iterate_lines(&file_content, find_section_start_and_end_callback, (void*)&sec_data);                 // find section start & end in file content

    if (!sec_data.found_last_section) {

        if (!sec_data.end)                                          
            sec_data.end = &file_content.data[file_content.len];
        
        for (size_t x = sec_data.headers_index; x < stack_size(&serializer->section_headers); x++) {        // add remaining header to file
            
            char current_header[STR_SEC_LEN] = {0};
            stack_peek_at(&serializer->section_headers, x, &current_header);
            
            const int indent_spaces = (x) * 2;                                                              // Calculate the number of spaces needed for indentation
            char indent_str[64] = {0};                                                                      // Create a string of spaces for indentation
            if (indent_spaces > 0 && indent_spaces < (int)sizeof(indent_str))
                memset(indent_str, ' ', indent_spaces);
            
            char header_str[STR_SEC_LEN *2] = {0};
            snprintf(header_str, sizeof(header_str), "\n%s%s:", indent_str, current_header);
            ds_insert_str(&file_content, sec_data.end - file_content.data, header_str);                   // Add the section header with proper indentation
            sec_data.start = sec_data.end + strlen(header_str);
            sec_data.end = sec_data.start;
        }
        sec_data.end = sec_data.start;
    }

    else if (!sec_data.end) {              // start found but not end -> assuming section is at end file    ([start] is always found if [found_last_section] id true)
        // LOG(Trace, "start found, but NOT end")
        sec_data.end = &file_content.data[file_content.len];
    }

    // LOG(Warn, "before: %s", file_content)
    sec_data.file_content = &file_content;
    ds_iterate_lines(&serializer->section_content, add_or_update_entry, (void*)&sec_data);
    // LOG(Warn, "after: %s", file_content)

    // Save data to file
    rewind(serializer->fp); // Go to beginning of file
    ftruncate(fileno(serializer->fp), 0); // Truncate the file to 0 length
    fwrite(file_content.data, 1, file_content.len, serializer->fp);
    fflush(serializer->fp); // Ensure all data is written

    ds_free(&file_content);
    // LOG(Debug, "Saved section")
}

// ============================================================================================================================================
// value parsing
// ============================================================================================================================================

typedef struct {
    const char*     key;
    const char*     format;
    void*           value;
    b8              found;
    dyn_str*        section_content;
} ds_iterator_data;


// ================================= set value =================================

    // Handle different format types
#define FORMAT_VALUE(value_to_use)                                                                                                 \
    if (strcmp(format, "%d") == 0)          snprintf(value_str, sizeof(value_str), format, *(int*)value_to_use);                   \
    else if (strcmp(format, "%u") == 0)     snprintf(value_str, sizeof(value_str), format, *(unsigned int*)value_to_use);          \
    else if (strcmp(format, "%ld") == 0)    snprintf(value_str, sizeof(value_str), format, *(long*)value_to_use);                  \
    else if (strcmp(format, "%lu") == 0)    snprintf(value_str, sizeof(value_str), format, *(unsigned long*)value_to_use);         \
    else if (strcmp(format, "%lld") == 0)   snprintf(value_str, sizeof(value_str), format, *(long long*)value_to_use);             \
    else if (strcmp(format, "%llu") == 0)   snprintf(value_str, sizeof(value_str), format, *(unsigned long long*)value_to_use);    \
    else if (strcmp(format, "%f") == 0)     snprintf(value_str, sizeof(value_str), format, *(float*)value_to_use);                 \
    else if (strcmp(format, "%lf") == 0)    snprintf(value_str, sizeof(value_str), format, *(double*)value_to_use);                \
    else if (strcmp(format, "%Lf") == 0)    snprintf(value_str, sizeof(value_str), format, *(long double*)value_to_use);           \
    else if (strcmp(format, "%s") == 0)     snprintf(value_str, sizeof(value_str), format, (char*)value_to_use);                   \
    else                                    snprintf(value_str, sizeof(value_str), format, *(handle*)value_to_use);

// 
b8 set_value_callback(const char* line, size_t len, void *user_data) {

    ds_iterator_data* loc_data = (ds_iterator_data*)user_data;

    const size_t key_len = strlen(loc_data->key);
    if (len <= key_len || memcmp(line, loc_data->key, key_len) != 0 || line[key_len] != ':')        // Check if this line starts with target key followed by a colon
        return true;

    // LOG(Trace, "found [%s] in [%.*s]", loc_data->key, len, line)

    size_t value_len = len - key_len;                                            // Calculate the full length of the line (including potential newline)
    size_t offset = (line - loc_data->section_content->data) + key_len;         // Calculate the offset of this line in the dynamic string
    while (offset < loc_data->section_content->len && (loc_data->section_content->data[offset] == ':' || loc_data->section_content->data[offset] == ' ')) {

        offset++;       // Include trailing ":" and space
        value_len--;
    }
    
    // Handle different types appropriately
    const char* format = loc_data->format;
    
    char value_str[STR_LINE_LEN] = {0};
    FORMAT_VALUE(loc_data->value);

    // Replace the old value strinf inside the line with the new one
    ds_remove_range(loc_data->section_content, offset, value_len);
    ds_insert_str(loc_data->section_content, offset, value_str);
    
    // LOG(Trace, "section_content [\n%s]", loc_data->section_content->data)
    loc_data->found = true;
    return false;
}

// tries to find a line containing the key, if found it will update the value, if not it will append a new line at the end
b8 set_value(SY* serializer, const char* key, const char* format, void* value) {
    
    if (!serializer || !key || !format || !value) return false;

    ds_iterator_data loc_data;
    loc_data.key = key;
    loc_data.format = format;
    loc_data.value = value;
    loc_data.found = false;
    loc_data.section_content = &serializer->section_content;

    // LOG(Trace, "serializer->section_content [%s]", serializer->section_content.data)
    ds_iterate_lines(&serializer->section_content, set_value_callback, (void*)&loc_data);
    
    if (!loc_data.found) {                      // append new line at end if not found
        
        // Key not found - append a new line
        char new_line[STR_LINE_LEN];
        snprintf(new_line, sizeof(new_line), "%s: ", key);
        
        // Append the formatted value
        char value_str[STR_LINE_LEN];
        FORMAT_VALUE(value);

        strcat(new_line, value_str);
        strcat(new_line, "\n");
        ds_append_str(&serializer->section_content, new_line);
    }
    return loc_data.found;
}

#undef FORMAT_VALUE

// ================================= get value =================================

b8 get_value_callback(const char* line, size_t len, void *user_data) {

    ds_iterator_data* loc_data = (ds_iterator_data*)user_data;

    // LOG(Trace, "searching for [%s] in [%.*s]", loc_data->key, (int)(strchr(line, '\n') - line), line)

    // Check if this line starts with target key followed by a colon
    if (len <= strlen(loc_data->key) || memcmp(line, loc_data->key, strlen(loc_data->key)) != 0 || line[strlen(loc_data->key)] != ':')
        return true;
        
    const char* value_start = line + strlen(loc_data->key) + 1;                     // Find the position after the colon
    while (value_start < line || (*value_start == ' ' || *value_start == '\t'))     // Skip any whitespace after the colon
        value_start++;
    
    // Extract the value
    char value_str[STR_LINE_LEN] = {0};
    size_t value_len = len - (value_start - line);
    if (value_len >= sizeof(value_str))                 // force length to be shorter than buffer
        value_len = sizeof(value_str) - 1;

    memcpy(value_str, value_start, value_len);
    value_str[value_len] = '\0';
    
    // LOG(Trace, "value_len [%s]", &value_str)
    loc_data->found = sscanf(value_str, loc_data->format, loc_data->value) == 1;          // Parse the value
    // LOG(Trace, "format [%s] value_len [%s]", loc_data->format, loc_data->value)

    return !loc_data->found;
}

// tries to find a line containing the key, if found it will return true and set [char* line] to the line containing the key
b8 get_value(SY* serializer, const char* key, const char* format, handle* value) {

    if (!serializer || !key || !format || !value) return false;

    ds_iterator_data loc_data;
    loc_data.key = key;
    loc_data.format = format;
    loc_data.value = value;
    loc_data.found = false;
    loc_data.section_content = NULL;

    ds_iterate_lines(&serializer->section_content, get_value_callback, (void*)&loc_data);
    return loc_data.found;
}


// ============================================================================================================================================
// serializer
// ============================================================================================================================================


// Core functions
b8 sy_init(SY* serializer, const char* dir_path, const char* file_name, const char* section_name, const serializer_option option) {
    
    ASSERT(dir_path != NULL, "", "failed to provide a directory path");
    ASSERT(file_name != NULL, "", "failed to provide a file name");
    ASSERT(section_name != NULL, "", "failed to provide a section name");

    if (!system_ensure_directory_exists(dir_path)) {
        LOG(Error, "failed to create directory: %s\n", dir_path);
        return false;
    }

    char loc_file_path[PATH_MAX];
    memset(loc_file_path, 0, sizeof(loc_file_path));


    // Build full path safely: dir_path + "/" + file_name
    size_t dir_len = strlen(dir_path);
    size_t file_len = strlen(file_name);

    if (dir_len + 1 + file_len + 1 > sizeof(loc_file_path)) {
        LOG(Error, "Path too long: %s/%s\n", dir_path, file_name);
        return false;
    }

    const int written = snprintf(loc_file_path, sizeof(loc_file_path), "%s/%s", dir_path, file_name);           // Use snprintf safely

    if (written < 0 || (size_t)written >= sizeof(loc_file_path)) {
        LOG(Error, "Path too long or snprintf failed: %s/%s\n", dir_path, file_name);
        return false;
    }

    system_ensure_file_exists(loc_file_path);

    serializer->fp = fopen(loc_file_path, "a+");                                                                 // Open file for reading (saving will happen later in shutdown)
    VALIDATE(serializer->fp, return false, "opened file [%s]", "Failed to open file [%s]", loc_file_path);

    serializer->current_indentation = 1;                                                                        // default to 1
    serializer->option = option;                                                                                // Store serializer settings
    stack_init(&serializer->section_headers, sizeof(char) * STR_SEC_LEN, 2);                                    // headers are char arrays with cap: STR_SEC_LEN
    i32 result = stack_push(&serializer->section_headers, section_name);
    if (result)
        LOG(Error, "result: %s", strerror(result));

    ds_init(&serializer->section_content);                                                                      // Initialize dynamic string buffer and parse initial section
    get_content_of_section(serializer);

    return true;
}


void sy_shutdown(SY* serializer) {

    if (serializer->option == SERIALIZER_OPTION_SAVE)       // dump content to file
        save_section(serializer);

    if (serializer->fp) {                                   // close file
        fclose(serializer->fp);
        serializer->fp = NULL;
    }
    ds_free(&serializer->section_content);
    stack_free(&serializer->section_headers);
}


// ============================================================================================================================================
// Subsection function
// ============================================================================================================================================

void sy_subsection_begin(SY* serializer, const char* name) {

    ASSERT(strlen(name) < STR_SEC_LEN, "", "Provided section name is to long [%s] may size [%u]", name, STR_SEC_LEN)

    if (serializer->option == SERIALIZER_OPTION_SAVE)           // dump content to file
        save_section(serializer);

    stack_push(&serializer->section_headers, name);
    serializer->current_indentation++;
    get_content_of_section(serializer);
}


void sy_subsection_end(SY* serializer) {
    
    if (serializer->option == SERIALIZER_OPTION_SAVE)           // dump content to file
        save_section(serializer);

    // switch name back to parent section
    stack_pop(&serializer->section_headers, NULL);              // remove last
    serializer->current_indentation--;
    get_content_of_section(serializer);
}


// ============================================================================================================================================
// serializer entry functions
// ============================================================================================================================================

#define PARSE_VALUE(format)                                                                                     \
    if (serializer->option == SERIALIZER_OPTION_SAVE)   set_value(serializer, key, format, (void*)value);       \
    else                                                get_value(serializer, key, format, (void*)value);

void sy_entry(SY* serializer, const char* key, void* value, const char* format)     { PARSE_VALUE(format) }

void sy_entry_int(SY* serializer, const char* key, int* value)                        { PARSE_VALUE("%d") }

void sy_entry_f32(SY* serializer, const char* key, f32* value)                        { PARSE_VALUE("%f") }

void sy_entry_b32(SY* serializer, const char* key, b32* value)                        { PARSE_VALUE("%u") }

void sy_entry_str(SY* serializer, const char* key, char* value, size_t buffer_size)   {

    if (serializer->option == SERIALIZER_OPTION_SAVE) {
        set_value(serializer, key, "%s", (void*)value);
    } else {
        char temp[STR_LINE_LEN] = {0};
        if (get_value(serializer, key, "%[^\n]", (handle*)&temp)) {
            strncpy(value, temp, buffer_size);
            value[buffer_size - 1] = '\0'; // Ensure null termination
        }
    }
}

#undef PARSE_VALUE


void sy_loop(SY* serializer, const char* name, void* data_structure, size_t element_size, sy_loop_callback_t callback, sy_loop_callback_at_t accessor, sy_loop_callback_append_t append, sy_loop_DS_size_callback_t data_structure_size) {
    
    // TODO: try to find the section containing [name]

    if (serializer->option == SERIALIZER_OPTION_SAVE) {

        const size_t DS_size = data_structure_size(data_structure);
        for (u64 x = 0; x < DS_size; x++) {

            // TODO: load content of current element in to section_content if available

            void* local_buffer = NULL;
            const i32 result = accessor(data_structure, x, local_buffer);
            VALIDATE(result == AT_SUCCESS && local_buffer, return, "", "Failed to access element at [%d] result [%s]", x, error_to_str(result))
            callback(serializer, local_buffer);

            // TODO: save section_content to file
        }
        return;
    }

    // only loading from here
    void* local_buffer = malloc(element_size);
    // while ( --- ) {  // TODO: iterate as long as elements are in file

        // TODO: load content of current element in to section_content
    
        memset(local_buffer, 0, element_size);
        callback(serializer, local_buffer);
        append(data_structure, local_buffer);           // append new element to back
    // }

}

