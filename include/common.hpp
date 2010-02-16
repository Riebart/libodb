#ifndef COMMON_HPP
#define COMMON_HPP

//for the fail macro
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


#define FAIL(str...) { \
    if (errno != 0) { \
        fprintf (stderr, "%s:%d: %m: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n");} \
    else { \
        fprintf (stderr, "%s:%d: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n");} \
    exit(EXIT_FAILURE); }

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
/// @param[in,out] x Variable to assign the location to.
/// @param[in] n Number of bytes to request.
#define SAFE_MALLOC(t, x, n) x = reinterpret_cast<t>(malloc(n)); if (!x) OOM();

#define NOT_IMPLEMENTED(str...) { \
        fprintf(stderr, "Function not yet implemented: %s\n", str); }\
 


#endif
