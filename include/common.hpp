#ifndef COMMON_HPP
#define COMMON_HPP

//for the fail macro
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "lock.hpp"

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

#define NOT_IMPLEMENTED(str...) { \
        fprintf(stderr, "Function not yet implemented: %s\n", str); }\
 


#endif
