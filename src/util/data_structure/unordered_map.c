#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_types.h"
#include "unordered_map.h"

#define LOAD_FACTOR_THRESHOLD   0.75
#define MAGIC                   0xDEADBEEF

#define VALIDATE(map)                                               \
    do {                                                            \
        if (!(map)) return AT_INVALID_ARGUMENT;                     \
        if ((map)->magic != MAGIC || !(map)->buckets || (map)->cap == 0) \
            return AT_NOT_INITIALIZED;                              \
        if (!(map)->hash_fn || !(map)->key_cmp_fn)                  \
            return AT_NOT_INITIALIZED;                              \
    } while (0)


// ------------------------------------------------------------------------------------------
// Predefined hash functions
// ------------------------------------------------------------------------------------------

size_t string_hash(const void* key) {
    const char* str = (const char*)key;
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

size_t func_ptr_hash(const void* key)                       { return ((size_t)key) >> 3; }

size_t ptr_hash(const void* key)                            { return (size_t)key; }

size_t u32_hash(const void* key)                            { return (size_t)*(const u32*)key; }

size_t u64_hash(const void* key) {
    u64 value = *(const u64*)key;
    return (size_t)(value ^ (value >> 32));
}

size_t i32_hash(const void* key)                            { return (size_t)*(const i32*)key; }

size_t i64_hash(const void* key) {
    i64 value = *(const i64*)key;
    return (size_t)(value ^ (value >> 32));
}


// ------------------------------------------------------------------------------------------
// Predefined comparison functions
// ------------------------------------------------------------------------------------------

int string_compare(const void* key1, const void* key2)      { return strcmp((const char*)key1, (const char*)key2); }

int ptr_compare(const void* key1, const void* key2)         { return (key1 > key2) - (key1 < key2); }

int func_ptr_compare(const void* key1, const void* key2)    { return (key1 == key2) ? 0 : 1; }

int u32_compare(const void* key1, const void* key2) {
    u32 a = *(const u32*)key1;
    u32 b = *(const u32*)key2;
    return (a > b) - (a < b);
}

int u64_compare(const void* key1, const void* key2) {
    u64 a = *(const u64*)key1;
    u64 b = *(const u64*)key2;
    return (a > b) - (a < b);
}

int i32_compare(const void* key1, const void* key2) {
    i32 a = *(const i32*)key1;
    i32 b = *(const i32*)key2;
    return (a > b) - (a < b);
}

int i64_compare(const void* key1, const void* key2) {
    i64 a = *(const i64*)key1;
    i64 b = *(const i64*)key2;
    return (a > b) - (a < b);
}


// ------------------------------------------------------------------------------------------
// Map implementation
// ------------------------------------------------------------------------------------------

// Map creation
i32 u_map_init(unordered_map* map, size_t capacity, hash_func hash_fn, key_compare_func key_cmp_fn) {
    if (capacity == 0 || !hash_fn || !key_cmp_fn) return AT_INVALID_ARGUMENT;
    
    map = malloc(sizeof(unordered_map));
    if (!map) return AT_MEMORY_ERROR;
    
    map->buckets = calloc(capacity, sizeof(node*));
    if (!map->buckets) {
        free(map);
        return AT_MEMORY_ERROR;
    }
    
    map->size = 0;
    map->cap = capacity;
    map->magic = MAGIC;
    map->hash_fn = hash_fn;
    map->key_cmp_fn = key_cmp_fn;
    
    return AT_SUCCESS;
}


i32 u_map_free(unordered_map* map) {
    VALIDATE(map);
    
    for (size_t i = 0; i < map->cap; i++) {
        node* current = map->buckets[i];
        while (current != NULL) {
            node* temp = current;
            current = current->next;
            free(temp);
        }
    }
    
    free(map->buckets);
    free(map);
    return AT_SUCCESS;
}




// Resize function
i32 u_map_resize(unordered_map* map) {
    VALIDATE(map);
    
    size_t new_capacity = map->cap * 2;
    node** new_buckets = calloc(new_capacity, sizeof(node*));
    if (!new_buckets) return AT_MEMORY_ERROR;
    
    // Rehash all elements
    for (size_t i = 0; i < map->cap; i++) {
        node* current = map->buckets[i];
        while (current != NULL) {
            node* next = current->next;
            
            // Rehash the key
            size_t new_index = map->hash_fn(current->key) % new_capacity;
            
            // Insert at head of new bucket
            current->next = new_buckets[new_index];
            new_buckets[new_index] = current;
            
            current = next;
        }
    }
    
    free(map->buckets);
    map->buckets = new_buckets;
    map->cap = new_capacity;
    return AT_SUCCESS;
}

// Insert function
i32 u_map_insert(unordered_map* map, void* key, void* value) {
    VALIDATE(map);
    
    // Check if we need to resize
    if ((double)map->size / map->cap > LOAD_FACTOR_THRESHOLD) {
        i32 res = u_map_resize(map);
        if (res != AT_SUCCESS) return res;
    }
    
    size_t index = map->hash_fn(key) % map->cap;
    node* current = map->buckets[index];
    
    // Check if key already exists
    while (current != NULL) {
        if (map->key_cmp_fn(current->key, key) == 0) {
            current->value = value; // Update existing value
            return AT_SUCCESS;
        }
        current = current->next;
    }
    
    // Create new node
    node* newnode = malloc(sizeof(node));
    if (!newnode) return AT_MEMORY_ERROR;
    
    newnode->key = key;
    newnode->value = value;
    newnode->next = map->buckets[index]; // Insert at head
    map->buckets[index] = newnode;
    map->size++;
    
    return AT_SUCCESS;
}

// Find function
i32 u_map_find(unordered_map* map, const void* key, void** value) {
    VALIDATE(map);
    if (!value) return AT_INVALID_ARGUMENT;
    
    size_t index = map->hash_fn(key) % map->cap;
    node* current = map->buckets[index];
    
    while (current != NULL) {
        if (map->key_cmp_fn(current->key, key) == 0) {
            *value = current->value;
            return AT_SUCCESS;
        }
        current = current->next;
    }
    
    return AT_ERROR; // Key not found
}

// Erase function
i32 u_map_erase(unordered_map* map, const void* key) {
    VALIDATE(map);
    
    size_t index = map->hash_fn(key) % map->cap;
    node* current = map->buckets[index];
    node* prev = NULL;
    
    while (current != NULL) {
        if (map->key_cmp_fn(current->key, key) == 0) {
            if (prev == NULL) {
                map->buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }
            
            free(current);
            map->size--;
            return AT_SUCCESS;
        }
        prev = current;
        current = current->next;
    }
    
    return AT_ERROR; // Key not found
}
