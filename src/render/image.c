#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glx.h>

// GL extensions header for glGenerateMipmap
#ifdef __APPLE__
    #include <OpenGL/glext.h>
#else
    #define GL_GLEXT_PROTOTYPES  // Add this line
    #include <GL/glext.h>
#endif

#include "util/io/logger.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// #define STB_IMAGE_RESIZE_IMPLEMENTATION  
// #include "stb_image_resize.h"

#ifdef Status
    #undef Status
#endif

#include "image.h"




#define MAGIC       0xDEAFBEEF

#define VALIDATE_IMAGE(i) do {                                  \
    if (!i || i->magic != MAGIC) return AT_INVALID_ARGUMENT;    \
} while (0)

// fallback declaration if glGenerateMipmap is not found
#ifndef GL_GLEXT_PROTOTYPES
    void glGenerateMipmap(GLenum target);
#endif



// Utility function to get bytes per pixel
u32 image_bytes_per_pixel(image_format format) {
    switch (format) {
        case IF_RGBA:    return 4;
        case IF_RGBA32F: return 16;
        default: return 0;
    }
}

// Convert image format to OpenGL internal format
static GLenum to_gl_internal_format(image_format format) {
    switch (format) {
        case IF_RGBA:    return GL_RGBA8;
        case IF_RGBA32F: return GL_RGBA32F;
        default: return GL_RGBA8;
    }
}

// Convert image format to OpenGL base format
static GLenum to_gl_base_format(image_format format) {
    switch (format) {
        case IF_RGBA:    return GL_RGBA;
        case IF_RGBA32F: return GL_RGBA;
        default: return GL_RGBA;
    }
}

// Convert image format to OpenGL data format
static GLenum to_gl_data_format(image_format format) {
    switch (format) {
        case IF_RGBA:    return GL_UNSIGNED_BYTE;
        case IF_RGBA32F: return GL_FLOAT;
        default: return GL_UNSIGNED_BYTE;
    }
}

// Common image allocation function
static i32 image_allocate_memory(image_t* image, void* data, extent_3D size, image_format format, bool mipmapped) {
    
    if (!image) return AT_INVALID_ARGUMENT;
    
    image->image_extent = size;
    image->format = format;
    image->mipmapped = mipmapped;
    
    glGenTextures(1, &image->textureID);
    glBindTexture(GL_TEXTURE_2D, image->textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Set pixel alignment
    GLint alignment = 1;
    if (size.width % 8 == 0) alignment = 8;
    else if (size.width % 4 == 0) alignment = 4;
    else if (size.width % 2 == 0) alignment = 2;
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, to_gl_internal_format(format), 
                 size.width, size.height, 0, 
                 to_gl_base_format(format), to_gl_data_format(format), data);
    
    // Generate mipmaps if requested
    if (mipmapped) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error [%d] during texture creation\n", err);
        glDeleteTextures(1, &image->textureID);
        return AT_ERROR;
    }
    
    image->magic = MAGIC;
    image->bindless_handle = 0;
    
    return AT_SUCCESS;
}

i32 image_create_from_data(image_t* image, void* data, extent_3D size, image_format format, bool mipmapped) {
    
    // image_t* image = (image_t*)malloc(sizeof(image_t));
    if (!image) return AT_ALREADY_INITIALIZED;
    memset(image, 0, sizeof(image_t));
    return image_allocate_memory(image, data, size, format, mipmapped);
}

i32 image_create_2d(image_t* image, void* data, u32 width, u32 height, image_format format, bool mipmapped) {

    extent_3D size = {width, height, 1};
    return image_create_from_data(image, data, size, format, mipmapped);
}

i32 image_create_from_file(image_t* image, const char* file_path, image_format format, bool mipmapped) {

    int width, height, channels;
    stbi_uc* data = stbi_load(file_path, &width, &height, &channels, STBI_rgb_alpha);
    VALIDATE(data, return AT_INVALID_ARGUMENT, "", "Could not load image from path [%s]\n", file_path)
    
    i32 result = image_create_2d(image, data, (u32)width, (u32)height, format, mipmapped);
    stbi_image_free(data);
    return result;
}

void image_free(image_t* image) {

    if (!image || image->magic != MAGIC) return;

    if (image->bindless_handle) {
        // If bindless texture is supported and created, make it non-resident. This requires GL_ARB_bindless_texture extension
        // glMakeTextureHandleNonResidentARB(image->bindless_handle);
    }
    
    glDeleteTextures(1, &image->textureID);
    memset(image, 0, sizeof(image_t));
    free(image);
}

u32 image_get_width(const image_t* image)       { return image ? image->image_extent.width : 0; }

u32 image_get_height(const image_t* image)      { return image ? image->image_extent.height : 0; }

ImTextureRef image_get_texture_id(image_t* image) {

    ImTextureRef result = {0};
    if (!image || image->magic != MAGIC) return result;
    
    result._TexID = (u64)image->textureID;
    return result;
}

i32 image_create_bindless_handle(image_t* image) {

    VALIDATE_IMAGE(image);
    if (image->bindless_handle) return AT_SUCCESS;
    
    // TODO: Check if bindless textures are supported. This requires GL_ARB_bindless_texture extension
    //   image->bindless_handle = glGetTextureHandleARB(image->textureID);
    //   glMakeTextureHandleResidentARB(image->bindless_handle);
    
    // For now I just set a placeholder since bindless textures are optional
    image->bindless_handle = (u64)image->textureID;
    return AT_SUCCESS;
}

void* image_decode(const void* data, u64 length, u32* out_width, u32* out_height) {
    
    int width, height, channels;
    stbi_uc* buffer = stbi_load_from_memory(
        (const stbi_uc*)data, 
        (int)length, 
        &width, &height, &channels, STBI_rgb_alpha
    );
    
    if (buffer) {
        *out_width = (u32)width;
        *out_height = (u32)height;
    }
    
    return buffer;
}
