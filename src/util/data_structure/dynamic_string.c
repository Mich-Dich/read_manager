
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>

#include "dynamic_string.h"


#define MAGIC 0xDEADBEEF  // Unique identifier for initialized strings

#define VALIDATE(s)                                                 \
    do {                                                                    \
        if (!(s)) return AT_INVALID_ARGUMENT;                               \
        if ((s)->magic != MAGIC || !(s)->data || (s)->cap == 0)     \
            return AT_NOT_INITIALIZED;                                      \
    } while (0)




// ============================================================================================================================================
// init
// ============================================================================================================================================

i32 ds_init(dyn_str* s) {

    if (s->magic == MAGIC) return AT_ALREADY_INITIALIZED;

    s->cap = 4096;
    s->data = malloc(s->cap);
    if (!s->data) return AT_MEMORY_ERROR;

    s->len = 0;
    s->data[0] = '\0';
    s->magic = MAGIC;
    return AT_SUCCESS;
}


i32 ds_init_s(dyn_str* s, const size_t needed_size) {

    if (s->magic == MAGIC) return AT_ALREADY_INITIALIZED;

    s->cap = needed_size + 64;      // add small buffer
    s->data = malloc(s->cap);
    if (!s->data) return AT_MEMORY_ERROR;

    s->len = 0;
    s->data[0] = '\0';
    s->magic = MAGIC;
    return AT_SUCCESS;
}


i32 ds_from_c_str(dyn_str* s, const char* text) {

    if (s->magic == MAGIC) return AT_ALREADY_INITIALIZED;
    if (!text) return AT_INVALID_ARGUMENT;

    const size_t len = strlen(text);
    const i32 result = ds_init_s(s, len);
    if (result != AT_SUCCESS) return result;

    return ds_append_str(s, text);
}


i32 ds_from_file(dyn_str* s, FILE* file) {

    if (s->magic == MAGIC) return AT_ALREADY_INITIALIZED;
    if (!file) return AT_INVALID_ARGUMENT;

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    if (file_size < 0) return AT_IO_ERROR;
    
    // Initialize with the file size
    i32 result = ds_init_s(s, (size_t)file_size);
    if (result != AT_SUCCESS) return result;
    
    // Read file content directly into the buffer
    s->len = fread(s->data, 1, (size_t)file_size, file);
    if (s->len != (size_t)file_size) {
        // Handle read error
        free(s->data);
        s->magic = 0;
        return AT_IO_ERROR;
    }
    
    // Ensure null termination
    s->data[s->len] = '\0';
    
    return AT_SUCCESS;
}



// ============================================================================================================================================
// free
// ============================================================================================================================================


i32 ds_free(dyn_str* s) {

    VALIDATE(s);

    free(s->data);
    s->data = NULL;
    s->len = s->cap = 0;
    s->magic = 0;
    return AT_SUCCESS;
}


i32 ds_clear(dyn_str* s) {

    VALIDATE(s);

    s->len = 0;
    s->data[0] = '\0';
    return AT_SUCCESS;
}


i32 ds_append_str(dyn_str* s, const char* text) {

    VALIDATE(s);
    if (!text)    return AT_INVALID_ARGUMENT;

    const size_t t_len = strlen(text);
    const i32 result = ds_ensure(s, t_len);
    if (result != AT_SUCCESS)   return result;

    memcpy(s->data + s->len, text, t_len);
    s->len += t_len;
    s->data[s->len] = '\0';
    return AT_SUCCESS;
}


i32 ds_append_char(dyn_str* s, const char c) {

    VALIDATE(s);

    const i32 result = ds_ensure(s, 1);
    if (result != AT_SUCCESS) return result;

    s->data[s->len++] = c;
    s->data[s->len] = '\0';
    return AT_SUCCESS;
}


i32 ds_append_fmt(dyn_str* s, i32* needed_space, const char* fmt, ...) {

    VALIDATE(s);
    if (!fmt) return AT_INVALID_ARGUMENT;

    va_list ap;
    va_start(ap, fmt);

    // determine required size
    va_list ap2;
    va_copy(ap2, ap);

    i32 local_buffer = 0;
    i32* needed_buffer = (needed_space) ? needed_space : &local_buffer;
    *needed_buffer = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    if (*needed_buffer < 0) {
        va_end(ap);
        return AT_FORMAT_ERROR;
    }

    if (*needed_buffer > 0) {

        const i32 result = ds_ensure(s, (size_t)needed_space);
        if (result != AT_SUCCESS) {
            va_end(ap);
            return result;
        }

        const i32 written = vsnprintf(s->data + s->len, s->cap - s->len, fmt, ap);
        if (written < 0) {
            va_end(ap);
            return AT_FORMAT_ERROR;
        }

        s->len += (size_t)written;
    }
    va_end(ap);
    return AT_SUCCESS;
}

// ============================================================================================================================================
// append
// ============================================================================================================================================


i32 ds_remove_range(dyn_str* s, const size_t pos, const size_t len) {

    VALIDATE(s);
    if (pos >= s->len) return AT_RANGE_ERROR;         // tried to remove something not inside the string
    if (pos + len > s->len) return AT_RANGE_ERROR;    // provided pos + length is longer than string

    memmove(s->data + pos, s->data + pos + len, s->len - pos -len + 1);
    s->len -= len;
    return AT_SUCCESS;
}


i32 ds_insert_str(dyn_str* s, const size_t pos, const char* str) {

    VALIDATE(s);
    if (!str) return AT_INVALID_ARGUMENT;
    if (pos > s->len) return AT_RANGE_ERROR;

    const size_t str_len = strlen(str);
    const i32 result = ds_ensure(s, str_len);
    if (result != AT_SUCCESS) return result;

    memmove(s->data + pos + str_len, s->data + pos, s->len - pos +1);       // make room
    memcpy(s->data + pos, str, str_len);                                    // copy string content
    s->len += str_len;
    return AT_SUCCESS;
}   

// ============================================================================================================================================
// compare
// ============================================================================================================================================


i32 ds_compare(const dyn_str* s1, const dyn_str* s2) {
    VALIDATE(s1);
    VALIDATE(s2);
    
    return strcmp(s1->data, s2->data);
}

i32 ds_compare_cstr(const dyn_str* s1, const char* s2) {
    VALIDATE(s1);
    if (!s2) return AT_INVALID_ARGUMENT;
    
    return strcmp(s1->data, s2);
}

// ============================================================================================================================================
// search
// ============================================================================================================================================


ssize_t ds_find_char(const dyn_str* s, char c, size_t start_pos) {
    VALIDATE(s);
    if (start_pos >= s->len) return -1;
    
    char* found = memchr(s->data + start_pos, c, s->len - start_pos);
    return found ? (ssize_t)(found - s->data) : -1;
}

ssize_t ds_find_str(const dyn_str* s, const char* substr, size_t start_pos) {
    VALIDATE(s);
    if (!substr) return -1;
    if (start_pos >= s->len) return -1;
    
    char* found = strstr(s->data + start_pos, substr);
    return found ? (ssize_t)(found - s->data) : -1;
}

ssize_t ds_find_last_char(const dyn_str* s, char c) {
    VALIDATE(s);
    
    for (ssize_t i = s->len - 1; i >= 0; i--) {
        if (s->data[i] == c) return i;
    }
    return -1;
}

ssize_t ds_find_last_str(const dyn_str* s, const char* substr) {
    VALIDATE(s);
    if (!substr) return -1;
    
    size_t substr_len = strlen(substr);
    if (substr_len == 0) return -1;
    if (substr_len > s->len) return -1;
    
    for (ssize_t i = s->len - substr_len; i >= 0; i--) {
        if (strncmp(s->data + i, substr, substr_len) == 0) return i;
    }
    return -1;
}

// ============================================================================================================================================
// substring
// ============================================================================================================================================


i32 ds_substring(const dyn_str* s, size_t start, size_t len, dyn_str* result) {
    VALIDATE(s);
    if (!result) return AT_INVALID_ARGUMENT;
    if (start >= s->len) return AT_RANGE_ERROR;
    if (start + len > s->len) len = s->len - start;
    
    // Initialize result string
    i32 init_result = ds_init_s(result, len);
    if (init_result != AT_SUCCESS) return init_result;
    
    // Copy the substring
    memcpy(result->data, s->data + start, len);
    result->len = len;
    result->data[len] = '\0';
    
    return AT_SUCCESS;
}

i32 ds_substring_from(const dyn_str* s, size_t start, dyn_str* result) {
    VALIDATE(s);
    if (!result) return AT_INVALID_ARGUMENT;
    if (start >= s->len) return AT_RANGE_ERROR;
    
    size_t len = s->len - start;
    return ds_substring(s, start, len, result);
}

// ============================================================================================================================================
// transformation
// ============================================================================================================================================


i32 ds_to_lowercase(dyn_str* s) {
    VALIDATE(s);
    
    for (size_t i = 0; i < s->len; i++) {
        if (s->data[i] >= 'A' && s->data[i] <= 'Z') {
            s->data[i] = s->data[i] + ('a' - 'A');
        }
    }
    return AT_SUCCESS;
}

i32 ds_to_uppercase(dyn_str* s) {
    VALIDATE(s);
    
    for (size_t i = 0; i < s->len; i++) {
        if (s->data[i] >= 'a' && s->data[i] <= 'z') {
            s->data[i] = s->data[i] - ('a' - 'A');
        }
    }
    return AT_SUCCESS;
}

i32 ds_reverse(dyn_str* s) {
    VALIDATE(s);
    
    for (size_t i = 0; i < s->len / 2; i++) {
        char temp = s->data[i];
        s->data[i] = s->data[s->len - 1 - i];
        s->data[s->len - 1 - i] = temp;
    }
    return AT_SUCCESS;
}

// ============================================================================================================================================
// trim
// ============================================================================================================================================


i32 ds_trim(dyn_str* s) {
    VALIDATE(s);
    
    i32 result = ds_trim_start(s);
    if (result != AT_SUCCESS) return result;
    
    return ds_trim_end(s);
}

i32 ds_trim_start(dyn_str* s) {
    VALIDATE(s);
    
    size_t leading_spaces = 0;
    while (leading_spaces < s->len && 
           (s->data[leading_spaces] == ' ' || 
            s->data[leading_spaces] == '\t' || 
            s->data[leading_spaces] == '\n' || 
            s->data[leading_spaces] == '\r')) {
        leading_spaces++;
    }
    
    if (leading_spaces > 0) {
        memmove(s->data, s->data + leading_spaces, s->len - leading_spaces + 1);
        s->len -= leading_spaces;
    }
    
    return AT_SUCCESS;
}

i32 ds_trim_end(dyn_str* s) {
    VALIDATE(s);
    
    size_t trailing_spaces = 0;
    while (trailing_spaces < s->len && 
           (s->data[s->len - 1 - trailing_spaces] == ' ' || 
            s->data[s->len - 1 - trailing_spaces] == '\t' || 
            s->data[s->len - 1 - trailing_spaces] == '\n' || 
            s->data[s->len - 1 - trailing_spaces] == '\r')) {
        trailing_spaces++;
    }
    
    if (trailing_spaces > 0) {
        s->len -= trailing_spaces;
        s->data[s->len] = '\0';
    }
    
    return AT_SUCCESS;
}

// ============================================================================================================================================
// replacement
// ============================================================================================================================================


i32 ds_replace(dyn_str* s, const char* old_str, const char* new_str) {
    VALIDATE(s);
    if (!old_str || !new_str) return AT_INVALID_ARGUMENT;
    
    const size_t old_len = strlen(old_str);
    if (old_len == 0) return AT_SUCCESS; // Nothing to replace
    
    // Find all occurrences
    dyn_str result;
    const i32 init_result = ds_init(&result);
    if (init_result != AT_SUCCESS) return init_result;
    
    size_t pos = 0;
    ssize_t found_pos;
    
    while ((found_pos = ds_find_str(s, old_str, pos)) != -1) {
        ds_append_fmt(&result, NULL, "%.*s", (i32)(found_pos - pos), s->data + pos);      // Append the part before the match
        ds_append_str(&result, new_str);            // Append the replacement
        pos = found_pos + old_len;                  // Move position after the match
    }
    
    ds_append_str(&result, s->data + pos);          // Append the remaining part
    ds_free(s);                                     // Swap the contents
    *s = result;
    
    return AT_SUCCESS;
}


i32 ds_replace_range(dyn_str* s, const size_t start_pos, const size_t length, const char* new_str) {
    VALIDATE(s);
    if (!new_str) return AT_INVALID_ARGUMENT;
    if (start_pos > s->len) return AT_RANGE_ERROR;

    // Calculate actual length to remove (don't go beyond string end)
    size_t remove_len = length;
    if (start_pos + remove_len > s->len) {
        remove_len = s->len - start_pos;
    }

    const size_t new_str_len = strlen(new_str);
    const size_t new_total_len = s->len - remove_len + new_str_len;

    // Ensure we have enough capacity
    if (new_total_len + 1 > s->cap) {
        size_t new_cap = s->cap;
        while (new_cap < new_total_len + 1) {
            new_cap *= 2;
        }
        char* new_data = realloc(s->data, new_cap);
        if (!new_data) return AT_MEMORY_ERROR;
        s->data = new_data;
        s->cap = new_cap;
    }

    // Move the tail of the string if needed
    if (remove_len != new_str_len) {
        size_t tail_start = start_pos + remove_len;
        size_t tail_length = s->len - tail_start;
        memmove(s->data + start_pos + new_str_len, 
                s->data + tail_start, 
                tail_length + 1); // +1 to include null terminator
    }

    // Copy the new string into place
    memcpy(s->data + start_pos, new_str, new_str_len);
    s->len = new_total_len;

    return AT_SUCCESS;
}


i32 ds_replace_char(dyn_str* s, char old_char, char new_char) {
    VALIDATE(s);
    
    for (size_t i = 0; i < s->len; i++) {
        if (s->data[i] == old_char) {
            s->data[i] = new_char;
        }
    }
    
    return AT_SUCCESS;
}

// ============================================================================================================================================
// conversion
// ============================================================================================================================================


i32 ds_to_int(const dyn_str* s, int* result) {
    VALIDATE(s);
    if (!result) return AT_INVALID_ARGUMENT;
    
    char* endptr;
    long value = strtol(s->data, &endptr, 10);
    
    if (endptr == s->data) {
        return AT_ERROR; // No conversion performed
    }
    
    if (value > INT_MAX || value < INT_MIN) {
        return AT_RANGE_ERROR;
    }
    
    *result = (int)value;
    return AT_SUCCESS;
}

i32 ds_to_double(const dyn_str* s, double* result) {
    VALIDATE(s);
    if (!result) return AT_INVALID_ARGUMENT;
    
    char* endptr;
    *result = strtod(s->data, &endptr);
    
    if (endptr == s->data) {
        return AT_ERROR; // No conversion performed
    }
    
    return AT_SUCCESS;
}

// ============================================================================================================================================
// util
// ============================================================================================================================================


b8 ds_starts_with(const dyn_str* s, const char* prefix) {
    VALIDATE(s);
    if (!prefix) return false;
    
    size_t prefix_len = strlen(prefix);
    if (prefix_len > s->len) return false;
    
    return strncmp(s->data, prefix, prefix_len) == 0;
}

b8 ds_ends_with(const dyn_str* s, const char* suffix) {
    VALIDATE(s);
    if (!suffix) return false;
    
    size_t suffix_len = strlen(suffix);
    if (suffix_len > s->len) return false;
    
    return strncmp(s->data + s->len - suffix_len, suffix, suffix_len) == 0;
}

b8 ds_contains(const dyn_str* s, const char* substr) {
    VALIDATE(s);
    if (!substr) return false;
    
    return strstr(s->data, substr) != NULL;
}

char ds_char_at(const dyn_str* s, size_t pos) {
    VALIDATE(s);
    if (pos >= s->len) return '\0';
    
    return s->data[pos];
}


i32 ds_ensure(dyn_str* s, const size_t extra) {

    VALIDATE(s);

    const size_t need = s->len + extra + 1;
    if (need > s->cap) {
        while (s->cap < need)
            s->cap *= 2;

        char* new_data = realloc(s->data, s->cap);
        if (!new_data)  return AT_MEMORY_ERROR;
        s->data = new_data;
    }
    return AT_SUCCESS;
}


i32 ds_iterate_lines(const dyn_str* s, b8 (*callback)(const char* line, size_t len, void* user_data), void* user_data) {
    
    VALIDATE(s);
    if (!callback) return AT_INVALID_ARGUMENT;
    
    char *start = s->data;
    char *end = s->data + s->len;
    
    while (start < end) {
        char* new_line = memchr(start, '\n', end - start);
        if (new_line == NULL) new_line = end;
        const size_t line_len = (size_t)(new_line - start);
        
        if (!callback(start, line_len, user_data))
            break;
            
        start = new_line + 1;
    }
    
    return AT_SUCCESS;
}
