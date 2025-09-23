#pragma once

#include "util/data_structure/data_types.h"

typedef struct {
    i16 year, month, day;
    i16 hour, minute, second;
    i16 millisec;
} system_time;



// ------------------------------------------------------------------------------------------------------------------
// timeing
// ------------------------------------------------------------------------------------------------------------------

// @brief Retrieves the current high-precision time using a monotonic clock.
//        The returned value is not affected by system clock adjustments,
//        making it suitable for measuring elapsed time accurately.
// @return Returns the current time in seconds as a floating-point value.
f64 get_precise_time();


// @brief Suspends the execution of the current thread for the specified duration
//        with high precision. Uses a hybrid approach: an initial `nanosleep`
//        for most of the duration and a busy-wait loop for the remaining time,
//        ensuring accurate sleep without overshooting.
// @param seconds The duration to sleep, in seconds (can include fractions).
void precise_sleep(const f64 seconds);


// ------------------------------------------------------------------------------------------------------------------
// executable path
// ------------------------------------------------------------------------------------------------------------------

// @brief Retrieves the current local system time, including year, month, day,
//        hour, minute, second, and millisecond.
//        Uses `gettimeofday` and `localtime_r` for thread-safe access.
// @return Returns a `system_time` struct containing the current local time.
system_time get_system_time();


// @brief Retrieves the absolute path to the directory where the current executable resides.
//        Uses `/proc/self/exe` on Linux to resolve the executable path and extracts its directory.
// @return Returns a pointer to a static null-terminated string containing the executable's directory.
//         Returns `NULL` if the path cannot be resolved.
const char* get_executable_path();

//
int get_executable_path_buf(char *out, size_t outlen);


// ------------------------------------------------------------------------------------------------------------------
// filesystem
// ------------------------------------------------------------------------------------------------------------------

//
b8 system_ensure_file_exists(const char* file_path);

//
b8 system_ensure_directory_exists(const char *path);
