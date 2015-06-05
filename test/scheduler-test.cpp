#include "scheduler.hpp"

using namespace libodb;

#include <assert.h>
#include <vector>
#include <set>
#include <stdio.h>
#include <sys/timeb.h>

// Use OpenMP as a comparison for multithreading speedup.
#include <omp.h>

#define TIME_DIFF(e, s) ((e).time - (s).time + 0.001 * ((int16_t)(e).millitm - (int16_t)(s).millitm))

#define ASSERT_UU(LHS, RHS)\
	if (LHS != RHS)\
    {\
		printf("%u %u\n", (LHS), (RHS));\
        fflush(stdout);\
		assert((LHS) == (RHS));\
	}

#define TEST_CLASS_BEGIN(name) \
	{\
	timeb ___class_start = {0}, ___class_end = {0};\
	timeb ___case_start = {0}, ___case_end = {0};\
	char* ___case_name = NULL;\
	char* ___class_name = (name);\
	double ___class_elapsed = 0.0;\
	double ___case_elapsed = 0.0;\
	bool ___case_ended = false;\
	ftime(&___class_start);\
	printf("= %s =\n", (((name) != NULL) ? (name) : ""));\
    fflush(stdout);

#define TEST_CASE_END() \
	if (!___case_ended && (___case_name != NULL))\
	{\
		ftime(&___case_end);\
		___case_elapsed = TIME_DIFF(___case_end, ___case_start);\
		printf("(end) %.5gs\n", \
		___case_elapsed);\
        fflush(stdout);\
		___case_ended = true;\
	}

#define TEST_CLASS_END() \
	TEST_CASE_END();\
	if (___class_name != NULL)\
	{\
		ftime(&___class_end);\
		___class_elapsed = TIME_DIFF(___class_end, ___class_start);\
		printf("= %s = %.5gs\n\n", \
		___class_name, \
		___class_elapsed);\
        fflush(stdout);\
	}\
	}

#define TEST_CASE(name) \
	TEST_CASE_END();\
	___case_ended = false;\
	ftime(&___case_start);\
	___case_name = name;\
	printf("(start) %s\n", (((name) != NULL) ? (name) : ""));\
    fflush(stdout);

#ifdef WIN32
#include <Windows.h>
#include <WinBase.h>
#include <Processthreadsapi.h>
#include <algorithm>

#define get_thread_id() GetCurrentThreadId()
#else
#include <unistd.h>

#define get_thread_id() pthread_self()
#define min(x,y) (((x) < (y)) ? (x) : (y))
#define sprintf_s sprintf
#endif

#ifdef CPP11THREADS
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

std::mutex mlock;
#endif

void sleep(int msec, bool dots)
{
#ifdef CPP11THREADS
	std::chrono::milliseconds d(min(msec, 1000));

	for (int i = 1000; i <= msec; i += 1000)
	{
		if (dots)
		{
			printf(".");
			fflush(stdout);
		}

		std::this_thread::sleep_for(d);
	}

	d = std::chrono::milliseconds(msec % 1000);
	std::this_thread::sleep_for(d);
#else
	int d = min(msec, 1000);

	for (int i = 1000; i <= msec; i += 1000)
	{
		if (dots)
		{
			printf(".");
			fflush(stdout);
		}

		usleep(1000 * d);
	}

	d = msec % 1000;
	usleep(1000 * d);
#endif
}

#ifdef CPP11THREADS
void test_atomic_bool()
{
    TEST_CLASS_BEGIN("Atomic variable lock-free status");
    bool b;
    
    {
        TEST_CASE("atomic<bool>")
        std::atomic<bool> a;
        b = std::atomic_is_lock_free(&a);
        printf("std atomic<bool> IS%s lock free\n", (b ? "" : " NOT"));
        fflush(stdout);
        TEST_CASE_END();
    }
    
    {
        TEST_CASE("atomic<bool>")
        std::atomic<uint64_t> a;
        b = std::atomic_is_lock_free(&a);
        printf("std atomic<uint64_t> IS%s lock free\n", (b ? "" : " NOT"));
        fflush(stdout);
        TEST_CASE_END();
    }
    
    TEST_CLASS_END();
}

SpinLock lock;

void* threadid_print_worker(void* a)
{
    lock.lock();
	printf("%u\n", get_thread_id());
    fflush(stdout);
    sleep(500, false);
    lock.unlock();
    return NULL;
}

void test_spinlocks()
{
    TEST_CLASS_BEGIN("SpinLock tests");
    
    TEST_CASE("Spawning Scheduler with four workers")
    Scheduler* sched = new Scheduler(4);
    
    TEST_CASE("Driver holding lock and adding four work units");
    lock.lock();

    sched->add_work(threadid_print_worker, NULL, NULL, Scheduler::NONE);
    sched->add_work(threadid_print_worker, NULL, NULL, Scheduler::NONE);
    sched->add_work(threadid_print_worker, NULL, NULL, Scheduler::NONE);
    sched->add_work(threadid_print_worker, NULL, NULL, Scheduler::NONE);
    
    TEST_CASE("Driver sleeping for five seconds, check CPU usage on host");
    sleep(5000, false);
    
    TEST_CASE("Driver unlocking and blocking, workers starting");
    lock.unlock();
    sched->block_until_done();
    
    TEST_CASE_END();
    delete sched;
    
    TEST_CLASS_END();
}

#define MLOCK() mlock.lock()
#define MUNLOCK() mlock.unlock()
#else
#include <pthread.h>

pthread_mutex_t mlock;

#define MLOCK() pthread_mutex_lock(&mlock);
#define MUNLOCK() pthread_mutex_unlock(&mlock);
#endif

std::vector<uint64_t> vec;
std::set<uint64_t> set;

void* threadid_worker(void* a)
{
	MLOCK();
    set.insert(get_thread_id());
	sleep(10, false);
	MUNLOCK();
	return NULL;
}

void create_destroy()
{
	TEST_CLASS_BEGIN("Simple create/destroy (5s sleep)");
	Scheduler* sched = new Scheduler(4);
	sleep(5000, false);
	delete sched;
	TEST_CLASS_END();
}

void thread_count(Scheduler* sched, int c)
{
	int min_required_threads = 4 * omp_get_max_threads();

	set.clear();
	sched->update_num_threads(c);

	for (int i = 0; i < c; i++)
	{
		sched->add_work(threadid_worker, NULL, NULL, Scheduler::NONE);
		sched->add_work(threadid_worker, NULL, NULL, Scheduler::NONE);
	}
	sched->block_until_done();

	if (set.size() != c)
	{
		if (c <= min_required_threads)
		{
			ASSERT_UU(set.size(), c);
		}
		else
		{
			printf("FAILED TO ALLOCATE %d THREADS\n", c);
            fflush(stdout);
		}
	}

	set.clear();
}

void thread_start_stop()
{
	TEST_CLASS_BEGIN("Worker thread count");
	Scheduler* sched = new Scheduler(0);

	TEST_CASE("1 thread");
	thread_count(sched, 1);
	TEST_CASE("2 threads");
	thread_count(sched, 2);
	TEST_CASE("1 thread");
	thread_count(sched, 1);
	TEST_CASE("5 threads");
	thread_count(sched, 5);
	TEST_CASE("1 thread");
	thread_count(sched, 1);
	TEST_CASE("10 threads");
	thread_count(sched, 10);
	TEST_CASE("64 threads");
	thread_count(sched, 64);
	TEST_CASE("128 threads");
	thread_count(sched, 128);
	TEST_CASE("512 threads");
	thread_count(sched, 512);

	TEST_CASE_END();
	delete sched;
	TEST_CLASS_END();
}

void* nop_workload(void* args)
{
	return (void*)&nop_workload;
}

void* spin_workload(void* countV)
{
	int count = *(int*)countV;
	uint64_t s = 0;

	for (int i = 0; i < count; i++)
	{
		s += i * s + i;
	}

	return (void*)s;
}

void load_lock(char* name, void* (*f)(void*), void* args, int n)
{
	TEST_CLASS_BEGIN(name);

	char buf[128];
	uint64_t done = 0;
	int64_t us1 = 0, uso;
	Scheduler* sched = new Scheduler(0);

	// Test single-threaded performance, with and without the scheduler.
	TEST_CASE("Unscheduled work items, 1 thread");
	for (int i = 0; i < n; i++)
	{
		us1 += (int64_t)f(args) + i;
	}

	for (int nt = 2; nt <= omp_get_max_threads(); nt++)
	{
		sprintf_s(buf, "Unscheduled work items, %d threads (OpenMP)", nt);
		TEST_CASE(buf);
		uso = 0;
#pragma omp parallel for num_threads(nt) reduction(+ : uso)
		for (int i = 0; i < n; i++)
		{
			uso += (int64_t)f(args) + i;
		}
		ASSERT_UU(us1, uso);
		TEST_CASE_END();
	}

	TEST_CASE("Add work items, 0 consumers");
	sched->update_num_threads(0);
	for (int i = 0; i < n; i++)
	{
		sched->add_work(f, args, NULL, Scheduler::NONE);
	}

	TEST_CASE("1 consumer");
	sched->update_num_threads(1);
	sched->block_until_done();
	ASSERT_UU(done + n, sched->get_num_complete());
	done += n;
	TEST_CASE_END();

	for (int nt = 2; nt <= omp_get_max_threads(); nt++)
	{
		sched->update_num_threads(0);
		for (int i = 0; i < n; i++)
		{
			sched->add_work(f, args, NULL, Scheduler::NONE);
		}

		sprintf_s(buf, "%d scheduled consumers", nt);
		TEST_CASE(buf);
		sched->update_num_threads(nt);
		sched->block_until_done();
		ASSERT_UU(done + n, sched->get_num_complete());
		done += n;
		TEST_CASE_END();
	}

	for (int nt = 1; nt <= omp_get_max_threads(); nt++)
	{
		sched->update_num_threads(nt);
		sprintf_s(buf, "%d consumers, simultaneous add-consume", nt);
		TEST_CASE(buf);
		for (int i = 0; i < n; i++)
		{
			sched->add_work(f, args, NULL, Scheduler::NONE);
		}

		sched->block_until_done();
		ASSERT_UU(done + n, sched->get_num_complete());
		done += n;
		TEST_CASE_END();
	}

	TEST_CASE_END();
	delete sched;
	TEST_CLASS_END();
}

// struct class_workload_args
// {
//     uint64_t id;
//     uint64_t num;
//     std::mutex* lock;
// 
// };
// 
// void* class_test(void* aV)
// {
//     struct class_workload_args* args = (struct class_workload_args*)aV;
// 
//     return NULL;
// }

void test_interference_class_correctness()
{
    Scheduler* sched = new Scheduler(2);
    delete sched;
}

int main(int argc, char** argv)
{
#ifdef CPP11THREADS
	test_atomic_bool();
	test_spinlocks();
#else
   pthread_mutex_init(&mlock, NULL);
#endif

	create_destroy();
	thread_start_stop();

	load_lock("100k Independent NOP workload performance", nop_workload, NULL, 100000);
	load_lock("1M Independent NOP workload performance", nop_workload, NULL, 1000000);

	int nspin;
	nspin = 1000;
	load_lock("1M Independent 1k-spinning workload performance", spin_workload, &nspin, 1000000);
	nspin = 10000;
    load_lock("1M Independent 10k-spinning workload performance", spin_workload, &nspin, 1000000);
    nspin = 100000;
    load_lock("1M Independent 100k-spinning workload performance", spin_workload, &nspin, 1000000);

	return 0;
}
