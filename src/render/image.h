#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "cimgui.h"
#include "util/data_structure/data_types.h"



typedef struct {
    u32 width, height, depth;
} extent_3D;

// Image formats
typedef enum {
    IF_NONE = 0,
    IF_RGBA,       // 8-bit per channel RGBA format
    IF_RGBA32F     // 32-bit floating-point per channel RGBA format
} image_format;

// Image structure
typedef struct {
    u32             textureID;
    u64             bindless_handle;
    extent_3D       image_extent;
    image_format    format;
    u32             magic;
    bool            mipmapped;
} image_t;


// Create image from raw data
i32 image_create_from_data(image_t* image, void* data, extent_3D size, image_format format, bool mipmapped);

// Create 2D image from raw data
i32 image_create_2d(image_t* image, void* data, u32 width, u32 height, image_format format, bool mipmapped);

// Create image from file
i32 image_create_from_file(image_t* image, const char* file_path, image_format format, bool mipmapped);

// Destroy image and release resources
void image_free(image_t* image);

// Get image width
u32 image_get_width(const image_t* image);

// Get image height  
u32 image_get_height(const image_t* image);

// Get ImGui texture ID
ImTextureRef image_get_texture_id(image_t* image);

// Create bindless texture handle (if supported)
i32 image_create_bindless_handle(image_t* image);

// Decode image from memory
void* image_decode(const void* data, u64 length, u32* out_width, u32* out_height);

// Utility function to get bytes per pixel
u32 image_bytes_per_pixel(image_format format);
 