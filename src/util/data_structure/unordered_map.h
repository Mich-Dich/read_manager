#pragma once

#include <stddef.h>
#include "data_types.h"

// Function pointer types for hash and comparison
typedef size_t (*hash_func)(const void* key);
typedef int (*key_compare_func)(const void* key1, const void* key2);

typedef struct node {
    void*               key;
    void*               value;
    struct node*        next;
} node;

typedef struct {
    node**              buckets;
    size_t              size;
    size_t              cap;
    u32                 magic;
    hash_func           hash_fn;
    key_compare_func    key_cmp_fn;
} unordered_map;


// Creation with custom hash and compare functions
i32 u_map_init(unordered_map* map, size_t capacity, hash_func hash_fn, key_compare_func key_cmp_fn);
i32 u_map_free(unordered_map* map);

// Basic operations
i32 u_map_resize(unordered_map* map);
i32 u_map_insert(unordered_map* map, void* key, void* value);
i32 u_map_find(unordered_map* map, const void* key, void** value);
i32 u_map_erase(unordered_map* map, const void* key);


// Predefined hash functions for common types
size_t string_hash(const void* key);
size_t ptr_hash(const void* key);
size_t func_ptr_hash(const void* key);
size_t u32_hash(const void* key);
size_t u64_hash(const void* key);
size_t i32_hash(const void* key);
size_t i64_hash(const void* key);

// Predefined comparison functions for common types
int string_compare(const void* key1, const void* key2);
int ptr_compare(const void* key1, const void* key2);
int func_ptr_compare(const void* key1, const void* key2);
int u32_compare(const void* key1, const void* key2);
int u64_compare(const void* key1, const void* key2);
int i32_compare(const void* key1, const void* key2);
int i64_compare(const void* key1, const void* key2);
