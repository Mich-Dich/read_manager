#pragma once

#include <stdbool.h>
#include <errno.h>

#include <inttypes.h>


// Primitive type definitions for consistent sizing across platforms

typedef uint8_t     u8;    					// 8-bit unsigned integer
typedef uint16_t    u16;   					// 16-bit unsigned integer
typedef uint32_t    u32;   					// 32-bit unsigned integer
typedef uint64_t    u64;            // 64-bit unsigned integer

typedef int8_t      i8;   					// 8-bit signed integer
typedef int16_t     i16;  					// 16-bit signed integer
typedef int32_t     i32;  					// 32-bit signed integer
typedef int64_t     i64;  					// 64-bit signed integer

typedef float       f32;			      // 32-bit floating point
typedef double      f64;			      // 64-bit floating point
typedef long double f128;   				// 128-bit floating point (platform dependent)

typedef bool        b8;
typedef int         b32;            // 32-bit bool (often used as flag)

typedef u64 handle;  				        // Generic handle type for OS resources



// portable static assert for C11 and fallback 
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  // C11
  #define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
  // fallback: negative array size causes compile-time error for false condition
  #define STATIC_ASSERT_GLUE(a, b) a##b
  #define STATIC_ASSERT_LINE(name, line) STATIC_ASSERT_GLUE(name, line)
  #define STATIC_ASSERT(cond, msg) typedef char STATIC_ASSERT_LINE(static_assertion_, __LINE__)[(cond) ? 1 : -1]
#endif

STATIC_ASSERT(sizeof(f32) == 4,   "Expected [f32] to be 4 byte");
STATIC_ASSERT(sizeof(f64) == 8,   "Expected [f64] to be 8 byte");
STATIC_ASSERT(sizeof(f128) == 16, "Expected [f128] to be 16 byte");

STATIC_ASSERT(sizeof(b8) == 1,    "Expected [b8] to be 1 byte");
STATIC_ASSERT(sizeof(b32) == 4,   "Expected [b32] to be 4 byte");


// Extension for asset files
#define AT_ASSET_EXTENTION			".atasset"

// Extension for project files
#define PROJECT_EXTENTION    		".atproj"

// Configuration file extensions
#define FILE_EXTENSION_CONFIG   	".yml"        	// Extension for YAML config files
#define FILE_EXTENSION_INI      	".ini"          // Extension for INI config files

// Directory structure macros
#define METADATA_DIR            	"metadata"      // Directory for metadata files
#define CONFIG_DIR              	"config"        // Directory for configuration files
#define CONTENT_DIR             	"content"       // Directory for content files
#define SOURCE_DIR              	"src"           // Directory for source code
// #define ASSET_DIR                   util_get_executable_path() / "assets"





// Macros for easier error handling

// Success code
#define AT_SUCCESS              0

// Error codes (using standard errno values where appropriate)
#define AT_ERROR                -1      // Generic error
#define AT_INVALID_ARGUMENT     EINVAL  // Invalid parameter passed
#define AT_MEMORY_ERROR         ENOMEM  // Memory allocation failed
#define AT_RANGE_ERROR          ERANGE  // Position/length out of valid range
#define AT_FORMAT_ERROR         EILSEQ  // Invalid format string
#define AT_NOT_INITIALIZED      EPERM   // Data not properly initialized
#define AT_ALREADY_INITIALIZED  EEXIST  // Data already initialized
#define AT_IO_ERROR             EIO     // Input/output error (for future file operations)


static inline const char* error_to_str(const i32 error_code) {

  switch (error_code) {
    case AT_SUCCESS:              return "Success";
    case AT_ERROR:                return "Generic error";
    case AT_INVALID_ARGUMENT:     return "Invalid parameter passed";
    case AT_MEMORY_ERROR:         return "Memory allocation failed";
    case AT_RANGE_ERROR:          return "Position/length out of valid range";
    case AT_FORMAT_ERROR:         return "Invalid format string";
    case AT_NOT_INITIALIZED:      return "Data not properly initialized";
    case AT_ALREADY_INITIALIZED:  return "Data already initialized";
    case AT_IO_ERROR:             return "Input/output error (for future file operations)";
    default:                      return "unknown error code";
  }
}
