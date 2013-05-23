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

/// Header file containing commonly used standard values and functions.
/// This file contains typical default values for pruning functions and other
///utilities.
/// @file utility.hpp

#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string.h>

#include "comparator.hpp"

/// Finds the maximum of two values using the ternary operator.
/// @param[in] x One of the values.
/// @param[in] y The other value.
/// @return The maximum of x and y.
#ifndef MAX
#define MAX(x, y) ( x > y ? x : y )
#endif

/// Finds the minimum of two values using the ternary operator.
/// @param[in] x One of the values.
/// @param[in] y The other value.
/// @return The minimum of x and y.
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

/// Finds the square of a value
/// @param[in] x Value to square.
/// @return The literal x*x
#ifndef SQUARE
#define SQUARE(x) ((x) * (x))
#endif

#ifndef LEN_V
#define LEN_V
inline uint32_t len_v(void* rawdata)
{
    return strlen((const char*)rawdata);
}
#endif

inline bool search(std::vector<void*>* marked, void* addr)
{
    int32_t start = 0, end = marked->size() - 1, midpoint = (start + end) / 2;;
    int64_t c;

    while (start <= end)
    {
        c = reinterpret_cast<int64_t>(addr) - reinterpret_cast<int64_t>(marked->at(midpoint));

        if (c == 0)
        {
            return true;
        }
        else if (c < 0)
        {
            end = midpoint - 1;
        }
        else
        {
            start = midpoint + 1;
        }

        midpoint = (start + end) / 2;
    }

    return false;
}

inline bool prune_false(void* rawdata)
{
    return false;
}

inline bool prune_true(void* rawdata)
{
    return true;
}

inline int32_t compare_addr_f(void* a, void* b)
{
    int64_t c = reinterpret_cast<int64_t>(a) - reinterpret_cast<int64_t>(b);

    if (c < 0)
    {
        return -1;
    }
    if (c > 0)
    {
        return 1;
    }
    return 0;
}

static CompareCust* compare_addr = new CompareCust(compare_addr_f);

#endif
