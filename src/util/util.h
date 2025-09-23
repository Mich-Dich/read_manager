#pragma once

#include "util/data_structure/data_types.h"


// @brief Extracts the variable name from a compound expression.
//        Useful for logging and debugging to extract meaningful names
//        from complex expressions containing pointers and structures.
// @param name The compound expression (e.g., "option->m_font_size")
// @return The extracted variable name (e.g., "m_font_size") or NULL if invalid
const char* util_extract_variable_name(const char* name);





// @brief Search for the first occurrence of needle inside the first range_len bytes of haystack (bounded search).
//        Works similarly to the standard strstr / strnstr semantics but never
//        reads past haystack + range_len or past the first NUL in haystack.
// @param haystack   The string to search in (may be longer than range_len). Must not be NULL.
// @param needle     The substring to search for. Must not be NULL. If empty ("") the function returns haystack (standard strstr behaviour).
// @param range_len  Maximum number of bytes from haystack to search (may be 0).
// @return Pointer to the first occurrence of needle within the bounded range of haystack, or NULL if needle is not found or on invalid input.
const char* str_search_range(const char* haystack, const char* needle, size_t range_len);

