
#include <string.h>
#include <stddef.h>

#include "util/util.h"



// Example usage:
// util_extract_variable_name("option->m_font_size") returns "m_font_size"
// util_extract_variable_name("window.window_width") returns "window_width"
// util_extract_variable_name("parameter_x") returns "parameter_x"
const char* util_extract_variable_name(const char* name) {
    
    if (name == NULL)
        return NULL;
    
    // Find the last occurrence of "->"
    const char* arrow_pos = strstr(name, "->");
    const char* dot_pos = strrchr(name, '.');
    const char* last_delimiter = NULL;                                      // Determine which delimiter comes last in the string
    
    if (arrow_pos && dot_pos)                                               // Both found, use the one that appears later in the string
        last_delimiter = (arrow_pos > dot_pos) ? arrow_pos : dot_pos;
    else if (arrow_pos)                                                     // Only "->" found
        last_delimiter = arrow_pos;
    else if (dot_pos)                                                       // Only "." found
        last_delimiter = dot_pos;
    else                                                                    // No delimiter found, return the whole string
        return name;
    
    // Return the part after the delimiter
    // For "->", we need to skip both characters
    if (last_delimiter == arrow_pos)
        return last_delimiter + 2; // Skip "->"
    else
        return last_delimiter + 1; // Skip "."
}


const char* str_search_range(const char* haystack, const char* needle, size_t range_len) {

    if (!haystack || !needle) return NULL;
    size_t needle_len = strlen(needle);
    if (needle_len == 0) return haystack;       // needle empty -> found at start (like strstr)
    if (range_len == 0) return NULL;

    // Find actual haystack length limited by range_len (don't read past range_len)
    size_t hay_len = 0;
    while (hay_len < range_len && haystack[hay_len] != '\0')
        hay_len++;

    if (needle_len > hay_len) return NULL;                                  // If needle is longer than the searchable region, no match possible

    // Search: for each possible start i, compare needle_len bytes
    size_t last_start = hay_len - needle_len;                               // safe because needle_len <= hay_len
    for (size_t i = 0; i <= last_start; ++i)
        if (memcmp(haystack + i, needle, needle_len) == 0)
            return haystack + i;

    return NULL;
}

