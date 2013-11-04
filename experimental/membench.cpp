//Look at the relationship between the MT/s and the cycle time/bank cycle
//time as reported by CPU-Z and the maximum throughput in the completely random workload case.
// > My machine is 27/39 clocks
// > Do this on my work machine and my Alienware for additional samples


/// OUTPUT FORMAT
// First line: Num-Threads, Num-Elements, Element-Size(bytes), Num-Lookups
//    The product of the first three gives total memory usage, the last one determines runtime. Longer runs gives less susceptibility to variance.
//    The numebr of lookups is a base, and is modulated to meet a target run time per stride.
// Output  Lines: LookupRatio, Stride, Speed(Lookups/second)
//    The program tries to normalize the time per stride. This is done by modulating the number of lookups.
//    The stride is reduced exponentially (with a base of 1/1.1), starting at 10570841.
//    As the stride reduces, the lookup speed increases, the ratio of the time of current run to last run is calcualted
//    And that is used to multiply the numebr of lookups to keep approximately the same amount of time per run.

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <omp.h>

#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>

typedef uint64_t et;

#define GAIT 1
#define NUM_ELEMENTS 100000000
#define STRIDE 10570841 // The 700,000-th prime!

//#define NOTDOT(i) ((argv[i][0] == '.') && (argv[i][1] == '\0'))

//void usage()
//{
//	printf("Usage: membench <number of worker threads> <number of 8-byte items per thread> <target runtime in seconds per stride> <number of accesses per location> <stride> <number of samples desired, logarithmically spaced>\n");
//	printf("\n       Defaults are: 4, 150-million (or 1.2GB of RAM per worker), 0.25s, 1, 10570841, 170\n");
//	printf("\n   Specifying a single dot for an option will indicate to use the default while skipping to the next option.\n");
//	fflush(stdout);
//}

//int getval(char* s, char* f)
//{
//	int v, n;
//	n = sscanf(s, f, &v);
//
//	if (n != 1)
//	{
//		fprintf(stderr, "\"%s\" could not be parsed.\n", s);
//		exit(1);
//	}
//
//	return v;
//}

int main(int argc, char** argv)
{
    // Set the target run-time (in seconds),
    // which it will attempt to meet based on the last run time.
    double target = 0.1;
	//int NUM_ELEMENTS = 150000000;
	//int GAIT = 1;
	//int STRIDE = 10570841;
    uint64_t num_lookups = 700000L;

    double gdt = 0.0;
    uint64_t gsum = 0;
	int num_threads = omp_get_num_procs();
    omp_set_num_threads(num_threads);
    printf("%d,%d,%d,%d\n", num_threads, NUM_ELEMENTS, sizeof(et), num_lookups);
    fflush(stdout);

    // Used for reducing the stride
    double sr = 1.0;

    // Used for increasing the number of lookups to get a long enough sample
    double lr = 1.0;

    et** bytes = (et**)malloc(num_threads * sizeof(et*));
    for (int th_id = 0 ; th_id < num_threads ; th_id++)
    {
        bytes[th_id] = (et*)malloc(sizeof(et) * NUM_ELEMENTS);
        for (int i = 0 ; i < NUM_ELEMENTS ; i++)
        {
            bytes[th_id][i] = (i > 0 ? i * bytes[th_id][i-1] : th_id * th_id);
        }
    }

    while (sr < STRIDE)
    {
        gdt = 0.0;
        num_lookups = (uint64_t)(num_lookups * lr);
        int stride = (int)(1.0 * STRIDE / sr);
        // Make it odd if it is more than 100
        if (stride > 100)
        {
            stride += 1 - (stride % 2);
        }

#pragma omp parallel
        {
            struct timeb t0, t1;
            int th_id = omp_get_thread_num();
            et sum = 0;
            ftime(&t0);

            int c = 0;
            int j;
            for (uint64_t i = 0 ; i < num_lookups ; i++)
            {
                sum += bytes[th_id][c];
                c = (c + stride) % NUM_ELEMENTS;

                if ((GAIT > 1) && (c < (NUM_ELEMENTS - GAIT)))
                {
                    for (j = 1 ; j < GAIT ; j++)
                    {
                        sum += bytes[th_id][c + 1];
                    }
                }
            }

            ftime(&t1);
            double dt = ((t1.time - t0.time) + (t1.millitm - t0.millitm) / 1000.0);

#pragma omp critical
            {
                gsum += sum;
                gdt += dt;
            }
        }

        // Average it out?
        gdt /= num_threads;
        uint32_t speed = (uint32_t)(num_threads * num_lookups / gdt);
        printf("%g,%d,%d\n", lr, stride, speed);
        fflush(stdout);

        sr *= 1.1;
        lr = target / (1.0 * num_lookups / (speed / num_threads));
    }

    //printf("%g lookups over %g elements of %d bytes each across %d threads\nTotal sum: %lu\nRun time: %g s => %g /s\n",
    //  (double)NUM_LOOKUPS, (double)NUM_ELEMENTS, sizeof(et), num_threads,
    //  gsum,
    //  gdt, num_threads * NUM_LOOKUPS / gdt);

    return 0;
}
