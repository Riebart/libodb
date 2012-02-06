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
#include "unittest.hpp"

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

TEST_OPT_PREAMBLE("comp-scheduler")
printf("  There is no testable output for these tests as the output contains\n  perforamnce metrics and details.\n");
TEST_OPT("Unscheduled no-op single-threaded perforamnce.")
TEST_OPT("Scheduled, synchronous, no-op multi-threaded performance.")
TEST_OPT("Scheduled, deferred, no-op multi-threaded performance.")
TEST_OPT("Unscheduled, single-threaded ODB insertions.")
TEST_OPT("Scheduled, multit-threaded ODB insertions.")
TEST_OPT_END()

TEST_CASES_BEGIN()
TEST_BEGIN(0)
{
    // A mid-size workload is about 1000 cycles and corresponds to about 200k/s on a Core2Duo 2.5GHz T9300.
    num_cycles = 1000;
    N = RUN_TIME / num_cycles;
    num_consumers = 2;
    num_spin_waits = 0;
    
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (uint64_t i = 0 ; i < N ; i++)
    {
        spin_work(num_cycles);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("= Unscheduled Performance Run =\n%g seconds\n%lu processed\n@ %g /s (%lu spins)\n\n",
           TIME_DIFF(),
           N,
           N / TIME_DIFF(),
           num_spin_waits * num_cycles);
}

TEST_BEGIN(1)
{
    // A mid-size workload is about 1000 cycles and corresponds to about 200k/s on a Core2Duo 2.5GHz T9300.
    num_cycles = 1000;
    N = RUN_TIME / num_cycles;
    num_consumers = 2;
    num_spin_waits = 0;
    
    struct timespec start, end;
    Scheduler* sched = new Scheduler(num_consumers - 1);
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (uint64_t i = 0 ; i < N ; i++)
    {
        sched->add_work(spin_work_v, &num_cycles, NULL, Scheduler::NONE);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("= Simultaneous Scheduled Performance Run (%d threads) =\n%g seconds of insertion\n",
           num_consumers,
           TIME_DIFF());

    sched->update_num_threads(num_consumers);
    sched->block_until_done();
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("%g seconds total\n%lu processed\n@ %g /s (%lu spins)\n",
           TIME_DIFF(),
           N,
           N / TIME_DIFF(),
           num_spin_waits * num_cycles);

    delete sched;
}

TEST_BEGIN(2)
{
    // A mid-size workload is about 1000 cycles and corresponds to about 200k/s on a Core2Duo 2.5GHz T9300.
    num_cycles = 1000;
    N = RUN_TIME / num_cycles;
    num_consumers = 2;
    num_spin_waits = 0;
    
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

TEST_BEGIN(3)
{
    // A mid-size workload is about 1000 cycles and corresponds to about 200k/s on a Core2Duo 2.5GHz T9300.
    num_cycles = 1000;
    N = RUN_TIME / num_cycles;
    num_consumers = 2;
    num_cycles = 100;
    num_indices = 10 * num_consumers;
    // A total count of 20-million uses about 1.5Gb of memory.
    // Modulating it so that we are as close to 20-million as possible, across
    // all index tables will maintain memory usage.
    N = 1200000 * (2.0 / num_indices);
    num_spin_waits = 0;
    
    struct timespec start, end;
    struct cmwc_state cmwc;

    double proc_time = 0;

    cmwc_init(&cmwc, 123456789);

    ODB odb(ODB::BANK_DS, sizeof(uint64_t), NULL);

    printf("= ODB Uncheduled Performance Run =\nCreating index tables... ");
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0 ; i < num_indices ; i++)
    {
        odb.create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_test4);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("done (%g s)\nInserting items... ", TIME_DIFF());
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint64_t i = 0 ; i < N ; i++)
    {
        uint64_t* v = (uint64_t*)malloc(sizeof(uint64_t));
        *v = cmwc_next(&cmwc) + ((uint64_t)(cmwc_next(&cmwc)) << 32);
        odb.add_data(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    proc_time += TIME_DIFF();
    printf("done (%g s)\n", TIME_DIFF());
    fflush(stdout);

    printf("Processing rate (%lu x %d @ %d): %g /s (%lu spins)\nDestroying... ",
           N,
           num_indices,
           num_cycles, (N * num_indices) / proc_time,
           num_spin_waits * num_cycles);
    fflush(stdout);
    
    printf("done\n\n");
}

TEST_BEGIN(4)
{
    num_cycles = 1000;
    N = RUN_TIME / num_cycles;
    num_consumers = 2;
    num_cycles = 100;
    num_indices = 10 * num_consumers;
    // A total count of 20-million uses about 1.5Gb of memory.
    // Modulating it so that we are as close to 20-million as possible, across
    // all index tables will maintain memory usage.
    N = 1200000 * (2.0 / num_indices);
    num_spin_waits = 0;
    
    struct timespec start, end;
    struct cmwc_state cmwc;

    double proc_time = 0;

    cmwc_init(&cmwc, 123456789);

    ODB odb(ODB::BANK_DS, sizeof(uint64_t), NULL);

    printf("= ODB Scheduled Performance Run (%d threads) =\nCreating index tables... ", num_consumers);
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0 ; i < num_indices ; i++)
    {
        odb.create_index(ODB::RED_BLACK_TREE, ODB::NONE, compare_test4);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("done (%g s)\nStarting scheduler... ", TIME_DIFF());
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);
    odb.start_scheduler(num_consumers - 1);
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("done (%g s)\nInserting items... ", TIME_DIFF());
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint64_t i = 0 ; i < N ; i++)
    {
        uint64_t* v = (uint64_t*)malloc(sizeof(uint64_t));
        *v = cmwc_next(&cmwc) + ((uint64_t)(cmwc_next(&cmwc)) << 32);
        odb.add_data(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    odb.start_scheduler(num_consumers);
    proc_time += TIME_DIFF();
    printf("done (%g s)\nBlocking... ", TIME_DIFF());
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);
    odb.block_until_done();
    clock_gettime(CLOCK_MONOTONIC, &end);

    proc_time += TIME_DIFF();
    printf("done (%g s)\n", TIME_DIFF());
    fflush(stdout);

    printf("Processing rate (%lu x %d @ %d): %g /s (%lu spins)\nDestroying... ",
           N,
           num_indices,
           num_cycles, (N * num_indices) / proc_time,
           num_spin_waits * num_cycles);
    fflush(stdout);
    
    printf("done\n\n");
}

TEST_CASES_END()
