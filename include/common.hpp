#ifndef COMMON_HPP
#define COMMON_HPP

//for the fail macro
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>

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

inline int64_t addr_compare(void* a, void* b)
{
    return (reinterpret_cast<int64_t>(b) - reinterpret_cast<int64_t>(a));
}

inline bool search(std::vector<void*>* marked, void* addr, int64_t (*compare)(void*, void*) = addr_compare)
{
    uint32_t start = 0, end = marked->size() - 1, midpoint = (start + end) / 2;;
    int64_t c;
    
    while (start <= end)
    {
        c = compare(addr, marked->at(midpoint));
        
        if (c == 0)
            return true;
        else if (c < 0)
            end = midpoint - 1;
        else
            start = midpoint + 1;
        
        midpoint = (start + end) / 2;
    }
    
    return false;
}

#endif
