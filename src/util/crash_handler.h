#pragma once

#include "util/data_structure/data_types.h"


typedef void (*crash_callback_t)(void);

// @brief Initializes the crash handler and sets up custom signal handlers
//        for critical signals such as SIGSEGV, SIGABRT, SIGFPE, SIGILL, and SIGBUS.
//        When a crash occurs, the handler captures a backtrace, resolves symbol names,
//        attempts source file and line resolution, and logs detailed crash information.
// @note On Linux, this uses `sigaction` with `SA_SIGINFO` to intercept crash signals.
// @return Returns `true` if all crash signal handlers were successfully installed,
//         otherwise returns `false`.
b8 crash_handler_init();


// @brief Shuts down the crash handler and restores the original signal handlers.
//        After calling this function, crash signals will no longer be intercepted,
//        and the default system behavior for those signals is restored.
void crash_handler_shutdown();


// @brief Subscribes a callback function to be executed when a crash occurs
// @param user_callback Function to call when a crash occurs (takes no parameters, returns void)
// @return A handle that can be used to unsubscribe from crash notifications
u32 crash_handler_subscribe_callback(crash_callback_t user_callback);


// @brief Unsubscribes a previously subscribed crash callback
// @param handle The handle returned by crash_handler_subscribe_crash_callback
void crash_handler_unsubscribe_callback(const u32 handle);
