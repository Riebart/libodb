#ifndef COMMON_HPP
#define COMMON_HPP

//for the fail macro
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>

#define PRINT_TRACE(str...) { \
    if (errno != 0) { \
        fprintf (stderr, "%s:%d: %m: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n");} \
    else { \
        fprintf (stderr, "%s:%d: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n");} \
    }

#define THROW_ERROR(code, str...) { \
    PRINT_TRACE(str); \
    throw code; \
    }
    
#define NOT_IMPLEMENTED(str) { \
    THROW_ERROR("NOT_IMP", str) \
    }

#define FAIL(str...) { \
    PRINT_TRACE(str); \
    abort(); }

#ifdef DEBUG
#define TRACE(str...) { \
        fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);\
        fprintf(stderr, str); }\
#else
#define DEBUG(str...) { }
#endif

#define OOM() FAIL("Out of memory, you twat.")

/// A macro that takes care of NULL-checking when mallocing memory.
/// @param[in] t Type to cast the location to. This is done using a reinterpret_cast
/// @param[out] x Variable to assign the location to.
/// @param[in] n Number of bytes to request.
#define SAFE_MALLOC(t, x, n)  { x = reinterpret_cast<t>(malloc(n)); if (!x) OOM(); } (void)0
#define SAFE_CALLOC(t, x, n, m) { x = reinterpret_cast<t>(calloc(n, m)); if (!x) OOM(); } (void)0
#define SAFE_REALLOC(t, x, n) { x = reinterpret_cast<t>(realloc(x, n)); if (!x) OOM(); } (void)0
 
#endif
