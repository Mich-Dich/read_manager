// stack.h
#pragma once

#include <stdlib.h>
#include "data_types.h"

typedef struct {
    void* data;         // Pointer to stack elements
    size_t size;        // Current number of elements
    size_t cap;         // Allocated capacity
    size_t elem_size;   // Size of each element in bytes
    u32 magic;          // Magic number for validation
} stack;


// Initializes a stack with a specific element size
i32 stack_init(stack* s, size_t elem_size, size_t initial_capacity);

// Frees the stack memory
i32 stack_free(stack* s);

// -------------------------------------------------------------------------------------
// Stack operations
// -------------------------------------------------------------------------------------

i32 stack_push(stack* s, const void* element);

i32 stack_pop(stack* s, void* out_element);

i32 stack_peek(const stack* s, void* out_element);

i32 stack_peek_at(const stack* s, const size_t index, void* out_element);

// -------------------------------------------------------------------------------------
// Utility functions
// -------------------------------------------------------------------------------------

size_t stack_size(const stack* s);

b8 stack_is_empty(const stack* s);

i32 stack_ensure_capacity(stack* s, size_t min_capacity);
