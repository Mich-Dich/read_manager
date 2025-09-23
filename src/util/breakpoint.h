#pragma once


#include <signal.h>  // for raise(), SIGTRAP 
#include <stdlib.h>  // for abort() 

#ifdef __cplusplus
extern "C" {
#endif

// If a previous BREAK_POINT macro exists (e.g. placeholder), remove it to avoid empty expansion 
#ifdef BREAK_POINT
  #undef BREAK_POINT
#endif

// Compiler / arch specific implementations 
#if defined(_MSC_VER)
    // MSVC 
    #include <intrin.h>
    #define BREAK_POINT() __debugbreak()

#elif defined(__clang__)
    // Clang: prefer builtin_debugtrap if available 
    #if __has_builtin(__builtin_debugtrap)
        #define BREAK_POINT() __builtin_debugtrap()
    #else
        #define BREAK_POINT() __builtin_trap()
    #endif

#elif defined(__GNUC__) || defined(__GNUG__)
    // GCC: builtin_trap is fine for both C and C++ 
    #define BREAK_POINT() __builtin_trap()

// Architecture-specific asm for ARM / AArch64 (only used if above compiler branches don't match) 
#elif defined(__aarch64__)
    // ARM64: BRK instruction 
    #define BREAK_POINT() do { __asm__ volatile("brk #0"); } while (0)

#elif defined(__arm__) || defined(__ARM_ARCH)
    // 32-bit ARM: BKPT instruction 
    #define BREAK_POINT() do { __asm__ volatile("bkpt #0"); } while (0)

#else
    // Fallback: POSIX SIGTRAP if available, otherwise abort() 
    #if defined(SIGTRAP)
        #define BREAK_POINT() raise(SIGTRAP)
    #else
        #define BREAK_POINT() abort()
    #endif
#endif


#ifdef __cplusplus
}
#endif
