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

/// Header file for SkipListI index type as well as its iterators.
/// @file skiplisti.hpp

#include "dll.hpp"

#ifndef SKIPLISTI_HPP
#define SKIPLISTI_HPP

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "index.hpp"

class LIBODB_API SkipListI : public Index
{
public:
    SkipListI(int (*)(void*, void*), void* (*)(void*, void*), bool);
    virtual void add(void*);

private:

};

#endif
