
#include <stdlib.h>
#include <string.h>

#include "darray.h"



#define MAGIC 0xDEADBEEFDEADBEEF

#define VALIDATE(d) \
    do { \
        if (!(d) || (d)->magic != MAGIC) return AT_INVALID_ARGUMENT; \
    } while (0)




// ============================================================================================================================================
// Initialization and cleanup
// ============================================================================================================================================


i32 darray_init(darray* d, size_t element_size) {

    return darray_init_with_capacity(d, element_size, 16);
}


i32 darray_init_with_capacity(darray* d, size_t element_size, size_t initial_capacity) {
    
    if (!d || element_size == 0) return AT_INVALID_ARGUMENT;
    if (d->magic == MAGIC) return AT_ALREADY_INITIALIZED;
    
    d->data = malloc(element_size * initial_capacity);
    if (!d->data) return AT_MEMORY_ERROR;
    
    d->count = 0;
    d->capacity = initial_capacity;
    d->element_size = element_size;
    d->magic = MAGIC;
    
    return AT_SUCCESS;
}


i32 darray_free(darray* d) {

    VALIDATE(d);
    
    free(d->data);
    d->data = NULL;
    d->count = d->capacity = d->element_size = 0;
    d->magic = 0;
    
    return AT_SUCCESS;
}

// ============================================================================================================================================
// Element access
// ============================================================================================================================================


i32 darray_get(const darray* d, size_t index, void* element) {

    VALIDATE(d);
    
    if (element)
        memcpy(element, (char*)d->data + (index * d->element_size), d->element_size);
    
    return AT_SUCCESS;
}

// ============================================================================================================================================
// Capacity
// ============================================================================================================================================


size_t darray_size(const darray* d) {
    if (!d || d->magic != MAGIC) return 0;
    return d->count;
}


size_t darray_capacity(const darray* d) {
    if (!d || d->magic != MAGIC) return 0;
    return d->capacity;
}


i32 darray_reserve(darray* d, size_t new_capacity) {

    VALIDATE(d);
    if (new_capacity <= d->capacity) return AT_SUCCESS;
    
    void* new_data = realloc(d->data, new_capacity * d->element_size);
    if (!new_data) return AT_MEMORY_ERROR;
    
    d->data = new_data;
    d->capacity = new_capacity;
    
    return AT_SUCCESS;
}


i32 darray_shrink_to_fit(darray* d) {

    VALIDATE(d);
    if (d->count == d->capacity) return AT_SUCCESS;
    
    void* new_data = realloc(d->data, d->count * d->element_size);
    if (!new_data && d->count > 0) return AT_MEMORY_ERROR;
    
    d->data = new_data;
    d->capacity = d->count;
    
    return AT_SUCCESS;
}

// ============================================================================================================================================
// Modifiers
// ============================================================================================================================================


i32 darray_push_back(darray* d, const void* element) {

    VALIDATE(d);
    if (!element) return AT_INVALID_ARGUMENT;
    
    if (d->count >= d->capacity) {
        size_t new_capacity = d->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        
        i32 result = darray_reserve(d, new_capacity);
        if (result != AT_SUCCESS) return result;
    }
    
    memcpy((char*)d->data + (d->count * d->element_size), element, d->element_size);
    d->count++;
    
    return AT_SUCCESS;
}


i32 darray_pop_back(darray* d, void* out_element) {

    VALIDATE(d);
    if (d->count == 0) return AT_ERROR;
    
    if (out_element) {
        memcpy(out_element, (char*)d->data + ((d->count - 1) * d->element_size), d->element_size);
    }
    
    d->count--;
    return AT_SUCCESS;
}


i32 darray_insert(darray* d, size_t index, const void* element) {

    VALIDATE(d);
    if (!element) return AT_INVALID_ARGUMENT;
    if (index > d->count) return AT_RANGE_ERROR;
    
    if (d->count >= d->capacity) {
        size_t new_capacity = d->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        
        i32 result = darray_reserve(d, new_capacity);
        if (result != AT_SUCCESS) return result;
    }
    
    // Move elements to make space
    if (index < d->count) {
        char* dest = (char*)d->data + ((index + 1) * d->element_size);
        char* src = (char*)d->data + (index * d->element_size);
        size_t bytes_to_move = (d->count - index) * d->element_size;
        memmove(dest, src, bytes_to_move);
    }
    
    // Insert new element
    memcpy((char*)d->data + (index * d->element_size), element, d->element_size);
    d->count++;
    
    return AT_SUCCESS;
}


i32 darray_erase(darray* d, size_t index) {

    VALIDATE(d);
    if (index >= d->count) return AT_RANGE_ERROR;
    
    // Move elements to fill the gap
    if (index < d->count - 1) {
        char* dest = (char*)d->data + (index * d->element_size);
        char* src = (char*)d->data + ((index + 1) * d->element_size);
        size_t bytes_to_move = (d->count - index - 1) * d->element_size;
        memmove(dest, src, bytes_to_move);
    }
    
    d->count--;
    return AT_SUCCESS;
}


i32 darray_clear(darray* d) {

    VALIDATE(d);
    d->count = 0;
    return AT_SUCCESS;
}


// ============================================================================================================================================
// Utility
// ============================================================================================================================================

i32 darray_resize(darray* d, size_t new_size, const void* default_value) {

    VALIDATE(d);
    
    if (new_size > d->capacity) {
        i32 result = darray_reserve(d, new_size);
        if (result != AT_SUCCESS) return result;
    }
    
    // Initialize new elements with default value if provided
    if (default_value && new_size > d->count) {
        for (size_t i = d->count; i < new_size; i++) {
            memcpy((char*)d->data + (i * d->element_size), default_value, d->element_size);
        }
    }
    
    d->count = new_size;
    return AT_SUCCESS;
}
