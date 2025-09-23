// stack.c
#include <string.h>
#include "stack.h"

#define STACK_MAGIC 0xDEADBEEF

#define VALIDATE(s) \
    do { \
        if (!(s) || (s)->magic != STACK_MAGIC) return AT_INVALID_ARGUMENT; \
    } while (0)




i32 stack_init(stack* s, size_t elem_size, size_t initial_capacity) {

    if (!s || elem_size == 0) return AT_INVALID_ARGUMENT;
    if (s->magic == STACK_MAGIC) return AT_ALREADY_INITIALIZED;

    if (initial_capacity == 0) initial_capacity = 16;

    s->data = malloc(elem_size * initial_capacity);
    if (!s->data) return AT_MEMORY_ERROR;

    s->size = 0;
    s->cap = initial_capacity;
    s->elem_size = elem_size;
    s->magic = STACK_MAGIC;

    return AT_SUCCESS;
}


i32 stack_free(stack* s) {

    VALIDATE(s);

    free(s->data);
    s->data = NULL;
    s->size = s->cap = s->elem_size = 0;
    s->magic = 0;

    return AT_SUCCESS;
}

// -------------------------------------------------------------------------------------
// Stack operations
// -------------------------------------------------------------------------------------


i32 stack_push(stack* s, const void* element) {

    VALIDATE(s);
    if (!element) return AT_INVALID_ARGUMENT;

    // Ensure capacity
    if (s->size >= s->cap) {
        size_t new_cap = s->cap * 2;
        void* new_data = realloc(s->data, new_cap * s->elem_size);
        if (!new_data) return AT_MEMORY_ERROR;
        
        s->data = new_data;
        s->cap = new_cap;
    }

    // Copy element to top of stack
    memcpy((char*)s->data + (s->size * s->elem_size), element, s->elem_size);
    s->size++;

    return AT_SUCCESS;
}


i32 stack_pop(stack* s, void* out_element) {

    VALIDATE(s);
    if (s->size == 0) return AT_ERROR;

    s->size--;
    if (out_element) {
        memcpy(out_element, (char*)s->data + (s->size * s->elem_size), s->elem_size);
    }

    return AT_SUCCESS;
}


i32 stack_peek(const stack* s, void* out_element) {

    VALIDATE(s);
    if (s->size == 0 || !out_element) return AT_ERROR;

    memcpy(out_element, (char*)s->data + ((s->size - 1) * s->elem_size), s->elem_size);
    return AT_SUCCESS;
}


i32 stack_peek_at(const stack* s, const size_t index, void* out_element) {

    VALIDATE(s);
    if (s->size == 0 || !out_element) return AT_ERROR;

    memcpy(out_element, (char*)s->data + (index * s->elem_size), s->elem_size);
    return AT_SUCCESS;
}



// -------------------------------------------------------------------------------------
// Utility functions
// -------------------------------------------------------------------------------------


size_t stack_size(const stack* s) {

    if (!s || s->magic != STACK_MAGIC) return 0;
    return s->size;
}


b8 stack_is_empty(const stack* s) {

    if (!s || s->magic != STACK_MAGIC) return true;
    return s->size == 0;
}


i32 stack_ensure_capacity(stack* s, size_t min_capacity) {

    VALIDATE(s);
    
    if (min_capacity <= s->cap) return AT_SUCCESS;
    
    size_t new_cap = s->cap;
    while (new_cap < min_capacity) {
        new_cap *= 2;
    }
    
    void* new_data = realloc(s->data, new_cap * s->elem_size);
    if (!new_data) return AT_MEMORY_ERROR;
    
    s->data = new_data;
    s->cap = new_cap;
    
    return AT_SUCCESS;
}
