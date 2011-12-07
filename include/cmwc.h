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
