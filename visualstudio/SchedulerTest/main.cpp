#include "scheduler.hpp"

#include <assert.h>
#include <vector>
#include <set>
#include <stdio.h>
#include <time.h>
#include <algorithm>

// Use OpenMP as a comparison for multithreading speedup.
#include <omp.h>

#define ASSERT_UU(LHS, RHS)\
	if (LHS != RHS)\
	{\
		fprintf(stderr, "%u %u\n", (LHS), (RHS));\
		assert((LHS) == (RHS));\
	}

#define STRINGER(a) #a

#define TEST_CLASS_BEGIN(name) \
	{\
	clock_t ___class_start = {0}, ___class_end = {0};\
	clock_t ___case_start = {0}, ___case_end = {0};\
	char* ___case_name = NULL;\
	char* ___class_name = (name);\
	double ___class_elapsed = 0.0;\
	double ___case_elapsed = 0.0;\
	bool ___case_ended = false;\
	___class_start = clock();\
	fprintf(stderr, "= %s =\n", (((name) != NULL) ? (name) : ""));

#define TEST_CASE_END() \
	if (!___case_ended && (___case_name != NULL))\
		{\
		___case_end = clock();\
		___case_elapsed = ((double)(___case_end - ___case_start) / CLOCKS_PER_SEC);\
		fprintf(stderr, "(end) %.5gs\n", \
		___case_elapsed);\
		___case_ended = true;\
	}

#define TEST_CLASS_END() \
	TEST_CASE_END();\
	if (___class_name != NULL)\
	{\
		___class_end = clock();\
		___class_elapsed = ((double)(___class_end - ___class_start) / CLOCKS_PER_SEC);\
		fprintf(stderr, "= %s = %.5gs\n\n", \
		___class_name, \
		___class_elapsed);\
	}\
	}

#define TEST_CASE(name) \
	TEST_CASE_END();\
	___case_ended = false;\
	___case_start = clock();\
	___case_name = name;\
	fprintf(stderr, "(start) %s\n", (((name) != NULL) ? (name) : ""));

#ifdef WIN32
#include <Windows.h>
#include <WinBase.h>
#include <Processthreadsapi.h>
#include <chrono>
#include <thread>
#include <mutex>

std::mutex mlock;
#else
#define sprintf_s sprintf
#endif

std::vector<uint64_t> vec;
std::set<uint64_t> set;

void sleep(int msec, bool dots)
{
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
}

void* threadid_worker(void* a)
{
	mlock.lock();
	set.insert(GetCurrentThreadId());
	sleep(10, false);
	mlock.unlock();
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
			fprintf(stderr, "FAILED TO ALLOCATE %d THREADS\n", c);
		}
	}

	set.clear();
}

void thread_start_stop()
{
	TEST_CLASS_BEGIN("Worker thread count");
	Scheduler* sched = new Scheduler(0);

	TEST_CASE("1");
	thread_count(sched, 1);
	TEST_CASE("2");
	thread_count(sched, 2);
	TEST_CASE("1");
	thread_count(sched, 1);
	TEST_CASE("5");
	thread_count(sched, 5);
	TEST_CASE("1");
	thread_count(sched, 1);
	TEST_CASE("10");
	thread_count(sched, 10);
	TEST_CASE("64");
	thread_count(sched, 64);
	TEST_CASE("128");
	thread_count(sched, 128);
	TEST_CASE("512");
	thread_count(sched, 512);

	TEST_CASE_END();
	delete sched;
	TEST_CLASS_END();
}

void* nop_workload(void* args)
{
	return &nop_workload;
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

	uint64_t done = 0;
	int64_t us1 = 0, uso;
	int64_t omp_local;
	Scheduler* sched = new Scheduler(0);

	// Test single-threaded performance, with and without the scheduler.
	TEST_CASE("Unscheduled work items, 1 thread");
	for (int i = 0; i < n; i++)
	{
		us1 += (int64_t)f(args) + i;
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

	char buf[128];
	for (int nt = 2; nt <= omp_get_max_threads(); nt++)
	{
		sprintf_s(buf, "Unscheduled work items, %d threads(OpenMP)", nt);
		TEST_CASE(buf);
		uso = 0;
#pragma omp parallel for num_threads(nt) reduction(+ : uso)
		for (int i = 0; i < n; i++)
		{
			uso += (int64_t)f(args) + i;
		}
		//printf(" - %d - ", uso);
		ASSERT_UU(us1, uso);
		TEST_CASE_END();

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

int main(int argc, char** argv)
{
	create_destroy();
	thread_start_stop();

	load_lock("100k Independent NOP workload performance", nop_workload, NULL, 100000);
	load_lock("1M Independent NOP workload performance", nop_workload, NULL, 1000000);

	int nspin;
	nspin = 1000;
	load_lock("1M Independent 1000-spinning workload performance", spin_workload, &nspin, 1000000);
	nspin = 10000;
	load_lock("1M Independent 10000-spinning workload performance", spin_workload, &nspin, 1000000);
	
	return 0;
}
