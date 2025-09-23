
#include <time.h>
#include <sys/time.h>
#include <libgen.h> 
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util/io/logger.h"
#include "system.h"



// ------------------------------------------------------------------------------------------------------------------
// timeing
// ------------------------------------------------------------------------------------------------------------------

f64 get_precise_time() {

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);                    // CLOCK_MONOTONIC is steady, not affected by system clock changes
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}


void precise_sleep(const f64 seconds) {

    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Calculate the end time
    struct timespec end = start;
    end.tv_sec += (time_t)seconds;
    end.tv_nsec += (long)((seconds - (time_t)seconds) * 1e9);
    
    // Normalize the nanoseconds field
    if (end.tv_nsec >= 1000000000L) {
        end.tv_sec += end.tv_nsec / 1000000000L;
        end.tv_nsec %= 1000000000L;
    }
    
    // First sleep for most of the time using nanosleep
    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t)seconds;
    sleep_time.tv_nsec = (long)((seconds - (time_t)seconds) * 1e9);
    
    // Subtract a small buffer to ensure we don't oversleep
    const long buffer_ns = 1000000L; // 1 ms buffer
    if (sleep_time.tv_nsec > buffer_ns) {
        sleep_time.tv_nsec -= buffer_ns;
    } else if (sleep_time.tv_sec > 0) {
        sleep_time.tv_sec -= 1;
        sleep_time.tv_nsec = 1000000000L - (buffer_ns - sleep_time.tv_nsec);
    } else {
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 0;
    }
    
    // Perform the initial sleep
    if (sleep_time.tv_sec > 0 || sleep_time.tv_nsec > 0) {
        nanosleep(&sleep_time, NULL);
    }
    
    // Busy-wait for the remaining time
    do {
        clock_gettime(CLOCK_MONOTONIC, &current);
    } while (current.tv_sec < end.tv_sec || (current.tv_sec == end.tv_sec && current.tv_nsec < end.tv_nsec));
}


system_time get_system_time() {

    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t t = tv.tv_sec;
    struct tm tm_local;
    localtime_r(&t, &tm_local);

    system_time out;
    out.year = tm_local.tm_year + 1900;
    out.month = tm_local.tm_mon + 1;
    out.day = tm_local.tm_mday;
    out.hour = tm_local.tm_hour;
    out.minute = tm_local.tm_min;
    out.second = tm_local.tm_sec;
    out.millisec = (int)(tv.tv_usec / 1000);
    return out;
}


// ------------------------------------------------------------------------------------------------------------------
// executable path
// ------------------------------------------------------------------------------------------------------------------

static char executable_path[PATH_MAX] = {0};


const char* get_executable_path() {
    if (executable_path[0] != '\0') {
        return executable_path;
    }

    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        perror("readlink");
        return NULL;
    }
    path[len] = '\0';

    char path_copy[PATH_MAX];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char* dir = dirname(path_copy);
    if (dir != NULL) {
        strncpy(executable_path, dir, sizeof(executable_path) - 1);
        executable_path[sizeof(executable_path) - 1] = '\0';
    }

    return executable_path;
}


int get_executable_path_buf(char *out, size_t outlen) {

    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1)
        return -1;

    path[len] = '\0';
    char tmp[PATH_MAX];
    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';
    char *d = dirname(tmp);
    if (!d)
        return -1;
        
    strncpy(out, d, outlen);
    out[outlen-1] = '\0';
    return 0;
}


// ------------------------------------------------------------------------------------------------------------------
// filesystem
// ------------------------------------------------------------------------------------------------------------------

// 
b8 system_ensure_directory_exists(const char *path) {
    
    if (!path || *path == '\0') return true; // empty path -> current dir 

    if (strlen(path) >= PATH_MAX) {
        LOG(Error, "path too long: %s\n", path);
        return false;
    }

    char tmp[PATH_MAX];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    struct stat st;
    // If path already exists and is a directory, success 
    if (stat(tmp, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return true;
        LOG(Error, "path exists but is not a directory: %s\n", tmp);
        return false;
    } else if (errno != ENOENT) {
        // unexpected stat() error 
        LOG(Error, "stat(%s) failed: %s\n", tmp, strerror(errno));
        return false;
    }

    // Walk the path, creating components as needed. Start at 1 to preserve leading '/' for absolute paths. 
    size_t len = strlen(tmp);
    for (size_t i = 1; i <= len; ++i) {
        if (tmp[i] == '/' || tmp[i] == '\0') {
            char save = tmp[i];
            tmp[i] = '\0';

            // skip if empty (can happen for leading slash) 
            if (tmp[0] != '\0') {
                if (stat(tmp, &st) != 0) {
                    if (errno == ENOENT) {
                        if (mkdir(tmp, 0755) != 0) {
                            if (errno != EEXIST) {
                                LOG(Error, "mkdir(%s) failed: %s\n", tmp, strerror(errno));
                                return false;
                            }
                        }
                    } else {
                        LOG(Error, "stat(%s) failed: %s\n", tmp, strerror(errno));
                        return false;
                    }
                } else {
                    // exists: ensure it is a directory 
                    if (!S_ISDIR(st.st_mode)) {
                        LOG(Error, "path component exists but not a directory: %s\n", tmp);
                        return false;
                    }
                }
            }

            tmp[i] = save;
        }
    }

    // Final check: path should now exist and be a directory 
    if (stat(path, &st) != 0) {
        LOG(Error, "final stat(%s) failed: %s\n", path, strerror(errno));
        return false;
    }
    if (!S_ISDIR(st.st_mode)) {
        LOG(Error, "final path exists but not a directory: %s\n", path);
        return false;
    }
    return true;
}

// Ensure file exists: create an empty file if missing, but DO NOT overwrite an existing file.
b8 system_ensure_file_exists(const char* file_path) {

    int fd = open(file_path, O_RDONLY);
    if (fd >= 0) {      // file already exists
        close(fd);

    } else {
        if (errno == ENOENT) {
            // try to create atomically: O_CREAT | O_EXCL won't overwrite existing file
            int create_fd = open(file_path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // 0644
            if (create_fd >= 0) {
                close(create_fd);                                       // created an empty file
            } else {
                if (errno == EEXIST) {
                    // race: somebody created it concurrently — treat as success

                } else {
                    LOG(Error, "failed to create file %s: %s\n", file_path, strerror(errno));
                    return false;
                }
            }
        } else {
            // some other error trying to stat/open the file
            LOG(Error, "error checking file %s: %s\n", file_path, strerror(errno));
            return false;
        }
    }
}
