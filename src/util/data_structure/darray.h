#pragma once

#include <stdlib.h>
#include <sys/types.h>
#include "util/data_structure/data_types.h"

typedef struct {
    void* data;
    size_t count;
    size_t capacity;
    size_t element_size;
    u64 magic;
} darray;


// ============================================================================================================================================
// Initialization and cleanup
// ============================================================================================================================================

// @brief Initializes a dynamic array with default capacity (16 elements)
// @param d Pointer to the darray structure to initialize
// @param element_size Size of each element in bytes
// @return AT_SUCCESS on success, error code on failure
i32 darray_init(darray* d, const size_t element_size);


// @brief Initializes a dynamic array with a specific initial capacity
// @param d Pointer to the darray structure to initialize
// @param element_size Size of each element in bytes
// @param initial_capacity Initial capacity of the array
// @return AT_SUCCESS on success, error code on failure
i32 darray_init_with_capacity(darray* d, const size_t element_size, const size_t initial_capacity);


// @brief Frees the memory used by the dynamic array
// @param d Pointer to the darray structure to free
// @return AT_SUCCESS on success, error code on failure
i32 darray_free(darray* d);


// ============================================================================================================================================
// Element access
// ============================================================================================================================================


// @brief Gets a pointer to the element at the specified index
// @param d Pointer to the darray structure
// @param index Index of the element to retrieve
// @param element Pointer to the element, or NULL if index is out of bounds
// @return AT_SUCCESS on success, error code on failure
i32 darray_get(const darray* d, const size_t index, void* element);


// @brief Accesses an element at the specified index with type safety
// @param d Pointer to the darray structure
// @param type Type of the elements in the array
// @param index Index of the element to retrieve
// @return The element at the specified index
#define darray_at(d, type, index) (((type*)((d)->data))[index])


// ============================================================================================================================================
// Capacity
// ============================================================================================================================================

// @brief Returns the number of elements in the array
// @param d Pointer to the darray structure
// @return Number of elements in the array
size_t darray_size(const darray* d);


// @brief Returns the current capacity of the array
// @param d Pointer to the darray structure
// @return Current capacity of the array
size_t darray_capacity(const darray* d);


// @brief Increases the capacity of the array to at least the specified value
// @param d Pointer to the darray structure
// @param new_capacity New minimum capacity for the array
// @return AT_SUCCESS on success, error code on failure
i32 darray_reserve(darray* d, const size_t new_capacity);


// @brief Reduces the capacity to fit the current number of elements
// @param d Pointer to the darray structure
// @return AT_SUCCESS on success, error code on failure
i32 darray_shrink_to_fit(darray* d);


// ============================================================================================================================================
// Modifiers
// ============================================================================================================================================

// @brief Adds an element to the end of the array
// @param d Pointer to the darray structure
// @param element Pointer to the element to add
// @return AT_SUCCESS on success, error code on failure
i32 darray_push_back(darray* d, const void* element);


// @brief Removes the last element from the array
// @param d Pointer to the darray structure
// @param out_element Optional pointer to store the removed element
// @return AT_SUCCESS on success, error code on failure
i32 darray_pop_back(darray* d, void* out_element);


// @brief Inserts an element at the specified position
// @param d Pointer to the darray structure
// @param index Position where the element should be inserted
// @param element Pointer to the element to insert
// @return AT_SUCCESS on success, error code on failure
i32 darray_insert(darray* d, const size_t index, const void* element);


// @brief Removes the element at the specified position
// @param d Pointer to the darray structure
// @param index Position of the element to remove
// @return AT_SUCCESS on success, error code on failure
i32 darray_erase(darray* d, const size_t index);


// @brief Removes all elements from the array
// @param d Pointer to the darray structure
// @return AT_SUCCESS on success, error code on failure
i32 darray_clear(darray* d);


// ============================================================================================================================================
// Utility
// ============================================================================================================================================

// @brief Resizes the array to the specified number of elements
// @param d Pointer to the darray structure
// @param new_size New size of the array
// @param default_value Optional pointer to a default value for new elements
// @return AT_SUCCESS on success, error code on failure
i32 darray_resize(darray* d, const size_t new_size, const void* default_value);

