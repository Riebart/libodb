#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include "scheduler.hpp"

void* spin_work(void* args)
{
    int num_cycles = *(int*)args;

    volatile uint64_t sss = (uint64_t)(&args);
    for (int iii = 0 ; iii < num_cycles; iii++)
    {
        sss += sss * iii;
    }

    return (void*)sss;
}

void test1(uint64_t N, int num_cycles, int num_consumers)
{
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (uint64_t i = 0 ; i < N ; i++)
    {
        spin_work(&num_cycles);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    printf("= Unscheduled Performance Run =\n%g seconds\n%lu processed\n@ %g /s\n",
           (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec)),
           N,
           N / (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec)));
}

void test2(uint64_t N, int num_cycles, int num_consumers)
{
    struct timespec start, end;
    Scheduler* sched = new Scheduler(num_consumers - 1);
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (uint64_t i = 0 ; i < N ; i++)
    {
        sched->add_work(spin_work, &num_cycles, NULL, Scheduler::NONE);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    printf("= Simultaneous Scheduled Performance Run (%d threads) =\n%g seconds of insertion\n",
           num_consumers,
           (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec)));
    
    sched->update_num_threads(num_consumers);
    sched->block_until_done();    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    printf("%g seconds total\n%lu processed\n@ %g /s\n",
           (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec)),
           N,
           N / (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec)));
    
    delete sched;
}

void test3(uint64_t N, int num_cycles, int num_consumers)
{
    struct timespec start, end;
    Scheduler* sched = new Scheduler(0);

    for (uint64_t i = 0 ; i < N ; i++)
    {
        sched->add_work(spin_work, &num_cycles, NULL, Scheduler::NONE);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    sched->update_num_threads(num_consumers);
    sched->block_until_done();
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    printf("= Deferred Scheduled Performance Run (%d threads) =\n%g seconds\n%lu processed\n@ %g /s\n",
        num_consumers,
        (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec)),
        N,
        N / (((int32_t)end.tv_sec - (int32_t)start.tv_sec) + 0.000000001 * ((int)end.tv_nsec - (int)start.tv_nsec)));

    delete sched;
}

int main (int argc, char ** argv)
{
    uint64_t N = 100000000;
    int num_cycles = 0;
    int num_consumers = 2;

/// TEST1: Unscheduled single-threaded performance - mid-size workload
    test1(N, num_cycles, num_consumers);
    printf("\n");

/// TEST2: Scheduled simultaneous multi-threaded performance - mid-size workload
    test2(N/3, num_cycles, num_consumers);
    printf("\n");

/// TEST3: Scheduled deferred performance - mid-size workload
    test3(N/3, num_cycles, num_consumers);
    printf("\n");

    return EXIT_SUCCESS;
}
