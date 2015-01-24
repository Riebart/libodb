/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2013 Michael Himbeault and Travis Friesen
 *
 */

/// Contains commonly used, or useful, macros and definitions.
/// @file common.hpp

#ifndef COMMON_HPP
#define COMMON_HPP

/// For the fail macro
#include <errno.h>

/// For fprintf
#include <stdio.h>

/// For malloc/calloc/realloc
#include <stdlib.h>

/// Prints a back trace from the location that it was called.
/// @param[in] str... A variable-length list of arguments to pass to fprintf
///that includes some specific strings to print along with the backtrace.
//! @todo Visual Studio warns that the __FILE__ and __LINE__ may be different here than gcc.
#define PRINT_TRACE_M(str, ...) { \
    if (errno != 0) { \
        fprintf (stderr, "%s:%d: %m: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n");} \
    else { \
        fprintf (stderr, "%s:%d: ",  __FILE__, __LINE__);\
        fprintf (stderr, str);\
        fprintf (stderr, "\n");} \
    }
#define PRINT_TRACE(str) PRINT_TRACE_M(str, "")

/// Throws an error code and prints a backtrace at the same time.
/// @param[in] code Code to throw. Can be anything that throw accepts.
/// @param[in] str... A variable-length list of arguments to pass to fprintf
///that includes some specific strings to print along with the backtrace.
#define THROW_ERROR_M(code, str, ...) { \
    PRINT_TRACE(str); \
    throw code; \
    }
#define THROW_ERROR(code, str) THROW_ERROR_M(code, str, "")

/// Throws a specific error when an unimplemented stub function is called.
/// @param[in] str... A variable-length list of arguments to pass to fprintf
///that includes some specific strings to print along with the backtrace.
#define NOT_IMPLEMENTED(str) { \
    THROW_ERROR("NOT_IMP", str) \
    }

/// Prints a backtrace and aborts execution at that point.
/// @param[in] str... A variable-length list of arguments to pass to fprintf
///that includes some specific strings to print along with the backtrace.
#define FAIL_M(str, ...) { \
    PRINT_TRACE(str); \
    abort(); }
#define FAIL(str) FAIL_M(str, "")

/// Defines a debugging output function that results in zero additional code if debugging is disabled.
///If debugging is enabled, it prints a backtrace and moves along.
/// @param[in] str... A variable-length list of arguments to pass to fprintf
///that includes some specific strings to print along with the backtrace.
/// @{
#ifdef DEBUG
#define DEBUG_PRINT(str, ...) PRINT_TRACE(str)
#else
#define DEBUG_PRINT(str, ...) { }
#endif
/// @}

/// A fail-type macro used when we are out of memory.
#define OOM() FAIL("Out of memory, you twat.")

/// A macro that takes care of NULL-checking when mallocing memory.
/// @param[in] t Type to cast the location to. This is done using a reinterpret_cast
/// @param[out] x Variable to assign the location to.
/// @param[in] n Number of bytes to request.
#define SAFE_MALLOC(t, x, n)  { (x) = reinterpret_cast<t>(malloc((n))); if (!(x)) OOM(); } (void)0
//#define SAFE_MALLOC(t, x, n)  { (x) = reinterpret_cast<t>(calloc(1, (n))); if (!(x)) OOM(); } (void)0

/// A macro that takes care of NULL-checking when callocing memory.
/// @param[in] t Type to cast the location to. This is done using a reinterpret_cast
/// @param[out] x Variable to assign the location to.
/// @param[in] n Number of blocks to request
/// @param[in] m Size of each block to request.
#define SAFE_CALLOC(t, x, n, m) { (x) = reinterpret_cast<t>(calloc((n), (m))); if (!(x)) OOM(); } (void)0

/// A macro that takes care of NULL-checking when callocing memory.
/// @param[in] t Type to cast the location to. This is done using a reinterpret_cast
/// @param[in] x_old Variable pointing to the location to be resized.
/// @param[out] x_new Variable to assign the location to.
/// @param[in] n New size in bytes to allocate.
/// @attention This does not take care of ensuring that a temporary variable is
///made so that proper checking can be done. It is possible to call this with x_old
///and x_new being the same location, but that is unadvisable. If the realloc fails
///in that situation, the memory pointed to by the given variable will be lost and
///and unrecoverable, resulting in a leak.
#define SAFE_REALLOC(t, x_old, x_new, n) { (x_new) = reinterpret_cast<t>(realloc((x_old), (n))); if (!(x_new)) OOM(); } (void)0

#endif
