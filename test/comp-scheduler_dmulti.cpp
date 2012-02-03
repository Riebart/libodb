/* MPL2.0 HEADER START
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MPL2.0 HEADER END
 *
 * Copyright 2010-2012 Michael Himbeault and Travis Friesen
 *
 */

#include <stdio.h>
#include <stdlib.h>

#if (CMAKE_COMPILER_SUITE_SUN)
#include <atomic.h>
#endif

#include <time.h>

#include "odb.hpp"
#include "scheduler.hpp"
#include "cmwc.h"

#define TIME_DIFF2(star, end) (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec))
#define TIME_DIFF() TIME_DIFF2(start, end)

// A run time of 1000000000 (1 billion) corresponds to approximately five seconds of processing on a C2D T9300 2.50GHz
#define RUN_TIME 2000000000

uint64_t N;
int num_cycles;
int num_indices;
int num_consumers;

volatile uint64_t num_spin_waits;

inline void* spin_work(int num_cycles)
{
    volatile uint64_t sss = (uint64_t)(&num_cycles);
    for (int iii = 0 ; iii < num_cycles; iii++)
    {
        sss += sss * iii;
    }

#if (CMAKE_COMPILER_SUITE_SUN)
    atomic_inc_64_nv(&num_spin_waits);
#elif (CMAKE_COMPILER_SUITE_GCC)
    __sync_fetch_and_add(&num_spin_waits, 1);
#else
#warning "Can't find a way to atomicly increment a uint64_t."
    int temp[-1];
#endif

    return (void*)sss;
}

void* spin_work_v(void* args)
{
    int num_cycles = *(int*)args;
    return spin_work(num_cycles);
}

int32_t compare_test4(void* aV, void* bV)
{
    uint64_t a = *(uint64_t*)aV;
    uint64_t b = *(uint64_t*)bV;

    spin_work(num_cycles);

    return (a < b ? -1 : (a == b ? 0 : 1));
}

void test()
{
    struct timespec start, end;
    Scheduler* sched = new Scheduler(0);
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (uint64_t i = 0 ; i < N ; i++)
    {
        sched->add_work(spin_work_v, &num_cycles, NULL, Scheduler::NONE);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("= Deferred Scheduled Performance Run (%d threads) =\n%g seconds of insertion\n",
           num_consumers,
           TIME_DIFF());

    clock_gettime(CLOCK_MONOTONIC, &start);
    sched->update_num_threads(num_consumers);
    sched->block_until_done();
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("%g seconds of processing\n%lu processed\n@ %g /s (%lu spins)\n",
           TIME_DIFF(),
           N,
           N / TIME_DIFF(),
           num_spin_waits * num_cycles);

    delete sched;
}

int main (int argc, char ** argv)
{
    // A mid-size workload is about 1000 cycles and corresponds to about 200k/s on a Core2Duo 2.5GHz T9300.
    num_cycles = 1000;
    N = RUN_TIME / num_cycles;
    num_consumers = 2;

    // TEST3: Scheduled deferred performance
    num_spin_waits = 0;
    test();
    printf("\n");

    return EXIT_SUCCESS;
}
