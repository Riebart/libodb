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

/// Implements a complementary multiply with carry psueodrandom number generator
///See http://en.wikipedia.org/wiki/Multiply-with-carry for information on these
///types of pseudorandom number generators. This is written to be a fast alternative
///to rand() where a lot of random numbers are required, and the quality of the
///values is not required to be high.
///
/// A possible use case for this PRNG is generating random data to feed into a
///system, or anywhere else where the random generation cannot be a significant
///consumer of processing cycles.
/// @file cmwc.h

#ifndef CMWC_H
#define CMWC_H

#include <stdlib.h>

/// Holds the state of the CMWC generator.
/// See http://en.wikipedia.org/wiki/Multiply-with-carry for details on the
///operation of this PRNG.
/// @bug This is probably not done properly. See http://en.wikipedia.org/wiki/Multiply-with-carry
struct cmwc_state
{
    /// A large value used to rapidly cycle the 64-bit value of x
    /// A careful selection of this initial value results in higher or lower
    ///quality output and periodicity.
    uint64_t a;

    /// The 64-bit value that is cycled rapidly to produce the output.
    /// Each time a value is generated, the lower 32 bits of this value are returned
    ///as the next value.
    uint64_t x;
};

/// Given a current state, generate the next random number.
/// @param[in] c The CMWC state.
/// @return The next random number.
int32_t cmwc_next(struct cmwc_state* c)
{
    c->x = (c->a * (c->x & (uint64_t)(0x00000000FFFFFFFF))) + (c->x >> 32);
    return (int32_t)(c->x);
}

/// Initialize the CMWC state with an explicit value for x.
/// @param[in,out] c The CMWC state to be initialized.
/// @param[in] x The initial value for the CMWC sequence.
void cmwc_init(struct cmwc_state* c, uint64_t x)
{
    c->a = 0xffffda61L;
    c->x = x;
}

/// Initialize the CMWC state with a random value for x chosen by rand()
/// @param[in,out] c The CMWC state to be initialized.
void cmwc_init(struct cmwc_state* c)
{
    c->a = 0xffffda61L;
    c->x = rand();
}

#endif
