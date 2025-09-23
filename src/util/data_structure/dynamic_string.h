#pragma once

#include <stdlib.h>
#include <sys/types.h>

#include "data_types.h"
   


typedef struct {
    char*       data;   // pointer to the dynamically allocated string buffer
    size_t      len;    // current length of the string (excluding null terminator)
    size_t      cap;    // allocated capacity of the buffer
    u32         magic;  // Magic number to verify initialization
} dyn_str;



// ============================================================================================================================================
// init
// ============================================================================================================================================

// @brief Initializes a dynamic string to an empty state,
//          allocates inital memory with default capacity
i32 ds_init(dyn_str* s);


// @brie Initializes a dynamic string with a specific size
// @param needed_size The minimum inital capacity to allocate
i32 ds_init_s(dyn_str* s, const size_t needed_size);


// @brief Initializes a dynamic stringfrom an existing C-string.
//          Allocates enough memory to hold the provided string.
// @param text The null-terminated string to initialize from.
i32 ds_from_c_str(dyn_str* s, const char* text);


i32 ds_from_file(dyn_str* s, FILE* file);

// ============================================================================================================================================
// free
// ============================================================================================================================================

// @brief Frees the memory used by the dynamic string
//          after this call, the string will be in an uninitialized state
i32 ds_free(dyn_str* s);


// @brief Clears the content of the dynamic string without freeing memory
//          Resets length to zero but maintains current capacity
i32 ds_clear(dyn_str* s);

// ============================================================================================================================================
// append
// ============================================================================================================================================

// @brief Appends a C-string to the end of the dynamic string
// @param test The null-terminated string to append
i32 ds_append_str(dyn_str* s, const char* text);


// @brief Appends a single character to the dynamic string
// @param c The character to append
i32 ds_append_char(dyn_str* s, const char c);


// @brief Appends a formate string to the dynamic string.
//          Works like printf-stale formating
// @param fmt A format string (printf-style)
// @param needed_space Returns how much space the formatted string needs
// @param ... The arguments to format
i32 ds_append_fmt(dyn_str* s, i32* needed_space, const char* fmt, ...);

// ============================================================================================================================================
// remove
// ============================================================================================================================================

// @brief Removes a range of characters from the dynamic string.
// @param pos Starting position of the range to remove.
// @param len Number of characters to remove.
i32 ds_remove_range(dyn_str* s, const size_t pos, const size_t len);


// @brief Inserts a string at a specific position in the dynamic string
// @param pos Position where the string should nbe inserted.
// @param str The null-terminated strings to insert
i32 ds_insert_str(dyn_str* s, const size_t pos, const char* str);


// ============================================================================================================================================
// compare
// ============================================================================================================================================

// @brief Compares two dynamic strings
// @return 0 if equal, negative if s1 < s2, positive if s1 > s2
i32 ds_compare(const dyn_str* s1, const dyn_str* s2);


// @brief Compares a dynamic string with a C-string
// @return 0 if equal, negative if s1 < s2, positive if s1 > s2
i32 ds_compare_cstr(const dyn_str* s1, const char* s2);

// ============================================================================================================================================
// search
// ============================================================================================================================================

// @brief Finds the first occurrence of a character in the dynamic string
// @return Position of character or -1 if not found
ssize_t ds_find_char(const dyn_str* s, char c, size_t start_pos);


// @brief Finds the first occurrence of a substring in the dynamic string
// @return Position of substring or -1 if not found
ssize_t ds_find_str(const dyn_str* s, const char* substr, size_t start_pos);


// @brief Finds the last occurrence of a character in the dynamic string
// @return Position of character or -1 if not found
ssize_t ds_find_last_char(const dyn_str* s, char c);


// @brief Finds the last occurrence of a substring in the dynamic string
// @return Position of substring or -1 if not found
ssize_t ds_find_last_str(const dyn_str* s, const char* substr);

// ============================================================================================================================================
// substring
// ============================================================================================================================================

// @brief Extracts a substring from the dynamic string
// @param start Starting position of the substring
// @param len Length of the substring to extract
// @return New dynamic string containing the substring
i32 ds_substring(const dyn_str* s, size_t start, size_t len, dyn_str* result);


// @brief Extracts a substring from start to the end of the string
i32 ds_substring_from(const dyn_str* s, size_t start, dyn_str* result);

// ============================================================================================================================================
// transformation
// ============================================================================================================================================

// @brief Converts all characters in the string to lowercase
i32 ds_to_lowercase(dyn_str* s);


// @brief Converts all characters in the string to uppercase
i32 ds_to_uppercase(dyn_str* s);


// @brief Reverses the string in place
i32 ds_reverse(dyn_str* s);

// ============================================================================================================================================
// trim
// ============================================================================================================================================

// @brief Removes whitespace from the beginning and end of the string
i32 ds_trim(dyn_str* s);


// @brief Removes whitespace from the beginning of the string
i32 ds_trim_start(dyn_str* s);


// @brief Removes whitespace from the end of the string
i32 ds_trim_end(dyn_str* s);

// ============================================================================================================================================
// replacement
// ============================================================================================================================================

// @brief Replaces all occurrences of a substring with another string
i32 ds_replace(dyn_str* s, const char* old_str, const char* new_str);


//
i32 ds_replace_range(dyn_str* s, const size_t start_pos, const size_t length, const char* new_str);


// @brief Replaces a character with another character
i32 ds_replace_char(dyn_str* s, char old_char, char new_char);

// ============================================================================================================================================
// conversion
// ============================================================================================================================================

// @brief Converts the dynamic string to an integer
// @param[out] result Pointer to store the converted integer
// @return AT_SUCCESS on success, error code on failure
i32 ds_to_int(const dyn_str* s, int* result);


// @brief Converts the dynamic string to a double
// @param[out] result Pointer to store the converted double
// @return AT_SUCCESS on success, error code on failure
i32 ds_to_double(const dyn_str* s, double* result);

// ============================================================================================================================================
// util
// ============================================================================================================================================

// @brief Checks if the string starts with the specified prefix
b8 ds_starts_with(const dyn_str* s, const char* prefix);


// @brief Checks if the string ends with the specified suffix
b8 ds_ends_with(const dyn_str* s, const char* suffix);


// @brief Checks if the string contains the specified substring
b8 ds_contains(const dyn_str* s, const char* substr);


// @brief Returns the character at a specific position
// @return The character at position pos, or 0 if out of bounds
char ds_char_at(const dyn_str* s, size_t pos);


// @brief Iterates over lines in a dyn_str.
// @param ds   Pointer to the dyn_str.
// @param callback  Function called for each line, receives (line_start, line_length, user_data).
//                  Return true to continue iteration, false to stop.
i32 ds_iterate_lines(const dyn_str* ds, b8 (*callback)(const char* line, size_t len, void* user_data), void* user_data);


// @brief Ensure hat the dynamic string has enough capacity to hold
//          at least "extra" more characters beyond its current length.
//          If necessary, reallocates the internal buffer.
i32 ds_ensure(dyn_str* s, const size_t extra);
