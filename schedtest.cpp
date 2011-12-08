#include <stdio.h>
#include <stdlib.h>

#include <sys/timeb.h>
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

int main (int argc, char ** argv)
{
    struct timeb start, end;
    uint64_t num_done;
    double run_time;
    int num_cycles;
    int num_threads;

/// TEST1
/// This compares the performance of the scheduler to a non-scheduled single-threaded execution of a mid-size workload.

    run_time = 10.0;
    num_cycles = 2500;

// First, the unscheduled run.

    num_done = 0;
    ftime(&start);
    ftime(&end);

    while (((int32_t)end.time - (int32_t)start.time) + 0.001 * ((int)end.millitm - (int)start.millitm) < run_time)
    {
        spin_work(&num_cycles);

        num_done++;
        ftime(&end);
    }

    ftime(&end);

    printf("= Unscheduled Performance Run =\n%g seconds\n%lu processed\n@ %g /s\n",
           ((int32_t)end.time - (int32_t)start.time) + 0.001 * ((int)end.millitm - (int)start.millitm),
           num_done,
           num_done / (((int32_t)end.time - (int32_t)start.time) + 0.001 * ((int)end.millitm - (int)start.millitm)));

// Second, the scheduled run.
    printf("\n");

    num_threads = 2;
    Scheduler* sched = new Scheduler(num_threads);
    num_done = 0;
    ftime(&start);
    ftime(&end);

    while (((int32_t)end.time - (int32_t)start.time) + 0.001 * ((int)end.millitm - (int)start.millitm) < run_time)
    {
        sched->add_work(spin_work, &num_cycles, NULL, Scheduler::NONE);

        num_done++;
        ftime(&end);
    }

    ftime(&end);
    printf("= Scheduled Performance Run (%d threads) =\n%g seconds\n%lu inserted\n%lu processed\n@ %g /s\n",
           num_threads,
           ((int32_t)end.time - (int32_t)start.time) + 0.001 * ((int)end.millitm - (int)start.millitm),
           num_done,
           sched->get_num_complete(),
           (sched->get_num_complete()) / (((int32_t)end.time - (int32_t)start.time) + 0.001 * ((int)end.millitm - (int)start.millitm)));

    delete sched;


/// TEST2
/// This ensures that it properly handles interference classes without, y'know, interfering.



    return EXIT_SUCCESS;
}
