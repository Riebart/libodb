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

#ifndef CMWC_H
#define CMWC_H

#include <stdlib.h>

struct cmwc_state
{
    uint64_t a;
    uint64_t x;
};

int32_t cmwc_next(struct cmwc_state* c)
{
    c->x = (c->a * (c->x & (uint64_t)(0x00000000FFFFFFFF))) + (c->x >> 32);
    return (int32_t)(c->x);
}

void cmwc_init(struct cmwc_state* c, uint64_t x)
{
    c->a = 0xffffda61L;
    c->x = x;
}

void cmwc_init(struct cmwc_state* c)
{
    c->a = 0xffffda61L;
    c->x = rand();
}

#endif
