#pragma once

#include <limits.h>
#include <stdio.h>

#include "util/data_structure/data_types.h"
#include "util/data_structure/dynamic_string.h"
#include "util/data_structure/stack.h"
#include "util/util.h"


typedef enum {
    SERIALIZER_OPTION_SAVE = 0,
    SERIALIZER_OPTION_LOAD,
} serializer_option;


#define STR_SEC_LEN     128

typedef struct {

    FILE*               fp;
    serializer_option   option;
    u32                 current_indentation;
    dyn_str             section_content;
    stack               section_headers;
} SY;


#define KEY_VALUE(variable)         util_extract_variable_name(#variable), &variable
#define KEY_VALUE_TYPE(variable)    KEY_VALUE(variable), TYPE_FORMAT(variable)

// Core functions
b8 sy_init(SY* serializer, const char* dir_path, const char* file_name, const char* section_name, const serializer_option option);
void sy_shutdown(SY* sy);

// Entry functions for different types
// Need value as pointer because value will be overwritten when option = LOAD

void sy_entry_int(SY* serializer, const char* key, int* value);
void sy_entry_f32(SY* serializer, const char* key, f32* value);
void sy_entry_b32(SY* serializer, const char* key, b32* value);
void sy_entry_str(SY* serializer, const char* key, char* value, size_t buffer_size);

// Generic version, user needs to define how he want to save the values
void sy_entry(SY* serializer, const char* key, void* value, const char* format);

// Subsection function
void sy_subsection_begin(SY* serializer, const char* name);
void sy_subsection_end(SY* serializer);


typedef bool (*sy_loop_callback_t)(SY* serializer, void* element);
typedef i32 (*sy_loop_callback_at_t)(void* data_structure, const u64 index, void* element);
typedef i32 (*sy_loop_callback_append_t)(void* data_structure, void* data);         // append [data] to END of [data_structure] specific to the users structure
typedef size_t (*sy_loop_DS_size_callback_t)(void* data_structure);

void sy_loop(SY* serializer, const char* name, void* data_structure, size_t element_size, sy_loop_callback_t callback, sy_loop_callback_at_t accessor, sy_loop_callback_append_t append, sy_loop_DS_size_callback_t data_structure_size);
