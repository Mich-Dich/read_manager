
#pragma once

#include <pthread.h>

#include "util/breakpoint.h"
#include "util/core_config.h"
#include "util/data_structure/data_types.h"


// @brief Enumeration of available log severity levels.
//        Levels are ordered by severity, with TRACE being the least severe
//        and FATAL being the most severe. Used to categorize log messages
//        and control logging output based on configured log level.
typedef enum {
    LOG_TYPE_TRACE = 0,
    LOG_TYPE_DEBUG,
    LOG_TYPE_INFO,
    LOG_TYPE_WARN,
    LOG_TYPE_ERROR,
    LOG_TYPE_FATAL,
} log_type;



// @brief Initializes the logging system with specified configuration.
//        Sets up log formatting, output destinations, and creates log file.
//        For multi-threaded applications, initializes background logging thread.
// @param log_msg_format Custom format string for log messages (see format tags)
// @param log_to_console Whether to output logs to console in addition to file
// @param log_dir Directory where log files will be stored
// @param log_file_name Base name for log files (without extension)
// @param use_append_mode True to append to existing log, false to overwrite
// @return True if initialization succeeded, false otherwise
b8 logger_init(const char* log_msg_format, const b8 log_to_console, const char* log_dir, const char* log_file_name, const b8 use_append_mode);


// @brief Gracefully shuts down the logging system.
//        Flushes any pending log messages, closes log files,
//        and cleans up resources. For multi-threaded applications,
//        stops the background logging thread.
// @return True if shutdown completed successfully
b8 logger_shutdown();


// @brief Internal function to log a message with specified severity and context.
//        Not intended for direct use - use the LOG_* macros instead.
// @param type Severity level of the message
// @param thread_id ID of the thread generating the message
// @param file_name Source file where the log call originated
// @param function_name Function where the log call originated
// @param line Line number in the source file
// @param message Format string for the log message (with optional varargs)
void log_message(log_type type, pthread_t thread_id, const char* file_name, const char* function_name, const int line, const char* message, ...);


// The format of log-messages can be customized with the following tags
// @note to format all following log-messages use: set_format()
// @note e.g. set_format("$B[$T] $L [$F] $C$E")
//
// @param $T time                    hh:mm:ss
// @param $H hour                    hh
// @param $M minute                  mm
// @param $S secund                  ss
// @param $J millisecused            jjj
//      
// @param $N data                    yyyy:mm:dd
// @param $Y data year               yyyy
// @param $O data month              mm
// @param $D data day                dd
//
// @param $Q thread                  Thread_id: 137575225550656 or a label if provided
// @param $F function name           main, foo
// @param $A file name               /home/workspace/test_cpp/src/main.cpp  /home/workspace/test_cpp/src/project.cpp
// @param $I only file name          main.cpp
// @param $G line                    1, 42
//
// @param $L log-level               add used log severity: [TRACE], [DEBUG] ... [FATAL]
// @param $B color begin             from here the color begins
// @param $E color end               from here the color will be reset
// @param $C text                    the message the user wants to print
// @param $Z new line                add a new line in the message format
void logger_set_format(const char* new_format);


// @brief Associates a human-readable label with a thread ID.
//        Registered labels will be used in log output instead of numeric thread IDs,
//        making logs easier to read in multi-threaded applications.
// @param thread_id The thread ID to label (typically from pthread_self())
// @param label Descriptive name for the thread
void logger_register_thread_label(pthread_t thread_id, const char* label);

#define LOGGER_REGISTER_THREAD_LABEL(label)     logger_register_thread_label((u64)pthread_self(), label);


void logger_remove_thread_label_by_id(pthread_t thread_id);


void logger_remove_thread_label_by_label(const char* label);


// This enables the different log levels (FATAL + ERROR are always on)
//  0 = FATAL + ERROR
//  1 = FATAL + ERROR + WARN
//  2 = FATAL + ERROR + WARN + INFO
//  3 = FATAL + ERROR + WARN + INFO + DEBUG
//  4 = FATAL + ERROR + WARN + INFO + DEBUG + TRACE
#define LOG_LEVEL_ENABLED           			4


#define LOG_Fatal(message, ...)                                         { log_message(LOG_TYPE_FATAL, pthread_self(), __FILE__, __func__, __LINE__, message, ##__VA_ARGS__); }
#define LOG_Error(message, ...)                                         { log_message(LOG_TYPE_ERROR, pthread_self(), __FILE__, __func__, __LINE__, message, ##__VA_ARGS__); }

#if LOG_LEVEL_ENABLED > 0
    #define LOG_Warn(message, ...)                                      { log_message(LOG_TYPE_WARN, pthread_self(), __FILE__, __func__, __LINE__, message, ##__VA_ARGS__); }
#else
    #define LOG_Warn(message, ...)                                      { }
#endif

#if LOG_LEVEL_ENABLED > 1
    #define LOG_Info(message, ...)                                      { log_message(LOG_TYPE_INFO, pthread_self(), __FILE__, __func__, __LINE__, message, ##__VA_ARGS__); }
#else
    #define LOG_Info(message, ...)                                      { }
#endif

#if LOG_LEVEL_ENABLED > 2
    #define LOG_Debug(message, ...)                                     { log_message(LOG_TYPE_DEBUG, pthread_self(), __FILE__, __func__, __LINE__, message, ##__VA_ARGS__); }
#else
    #define LOG_Debug(message, ...)                                     { }
#endif


#if LOG_LEVEL_ENABLED > 3
    #define LOG_Trace(message, ...)                                     { log_message(LOG_TYPE_TRACE, pthread_self(), __FILE__, __func__, __LINE__, message, ##__VA_ARGS__); }
    #define LOG_INIT                                                    LOG(Trace, "init")
    #define LOG_SHUTDOWN                                                LOG(Trace, "shutdown")
#else
    #define LOG_Trace(message, ...)                                     { }
    #define LOG_INIT
    #define LOG_SHUTDOWN
#endif

#define LOG(severity, message, ...)                                     LOG_##severity(message, ##__VA_ARGS__)



#if ENABLE_LOGGING_FOR_VALIDATION
    #define VALIDATE(expr, return_cmd, success_msg, failure_msg, ...)   \
        if (expr) { LOG(Trace, success_msg, ##__VA_ARGS__) }            \
        else {                                                          \
            LOG(Warn, failure_msg, ##__VA_ARGS__)                       \
            return_cmd;                                                 \
        }

    #define VALIDATE_S(expr, return_cmd)                                \
        if (!(expr)) {                                                  \
            LOG(Error, "Validation failed for [%s]", ##expr)            \
            return_cmd;                                                 \
        }
#else
    #define VALIDATE(expr, return_cmd, success_msg, failure_msg, ...)   if (!(expr)) { return_cmd; }
    #define VALIDATE_S(expr, return_cmd, success_msg, failure_msg)      if (!(expr)) { return_cmd; }
#endif



#if ENABLE_LOGGING_FOR_ASSERTS
    #define ASSERT(expr, success_msg, failure_msg, ...)                 \
        if (expr) { LOG(Trace, success_msg, ##__VA_ARGS__) }            \
        else {                                                          \
            LOG(Fatal, failure_msg, ##__VA_ARGS__)                      \
            BREAK_POINT();                                              \
        }
    #define ASSERT_S(expr)                                              \
        if (!(expr)) {                                                  \
            LOG(Fatal, "Assert failed for [%s]", ##expr)                \
            BREAK_POINT();                                              \
        }
#else
    #define ASSERT(expr, success_msg, failure_msg, ...)                 if (!(expr)) { BREAK_POINT(); }
    #define ASSERT_S(expr)                                              if (!(expr)) { BREAK_POINT(); }
#endif

// extra short version, does not log anything just test an expression
#define ASSERT_SS(expr)                                                 if (!(expr)) { BREAK_POINT(); }
