#include "../Source/PriorityQueue.h"
#include "../Source/Heap.h"
#include "gtest/gtest.h"
#include <queue>
#include <array>
#include <thread>
#include <random>
#include <vector>
#include <atomic>

//using namespace ::Nirvana;
using namespace std;
using ::Nirvana::Core::PriorityQueue;
using ::Nirvana::Core::RandomGen;

namespace TestPriorityQueue {

template <class PQ>
class TestPriorityQueue :
	public ::testing::Test
{
protected:
	TestPriorityQueue ()
	{}

	virtual ~TestPriorityQueue ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
#ifndef UNIT_TEST
		::Nirvana::Core::Heap::initialize ();
#endif
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
#ifndef UNIT_TEST
		::Nirvana::Core::Heap::terminate ();
#endif
	}
};

struct StdNode
{
	unsigned deadline;
	void* value;

	bool operator < (const StdNode& rhs) const
	{
		return deadline > rhs.deadline;
	}
};

typedef std::priority_queue <StdNode> StdPriorityQueue;

static const unsigned PTR_ALIGN = 4;

typedef ::testing::Types <
	PriorityQueue <1>,
	PriorityQueue <2>, 
	PriorityQueue <4>,
	PriorityQueue <16>
> MaxLevel;

TYPED_TEST_CASE (TestPriorityQueue, MaxLevel);

TYPED_TEST (TestPriorityQueue, SingleThread)
{
	TypeParam queue;
	StdPriorityQueue queue_std;
	RandomGen rndgen;
	uniform_int_distribution <int> distr;

	ASSERT_FALSE (queue.delete_min ());

	static const int MAX_COUNT = 1000;
	for (int i = MAX_COUNT; i > 0; --i) {
		unsigned deadline = distr (rndgen);
		StdNode node;
		node.deadline = deadline;
		node.value = (void*)(intptr_t)((deadline + 1) * PTR_ALIGN);
		queue.insert (deadline, node.value, rndgen);
		queue_std.push (node);
	}

	for (int i = MAX_COUNT; i > 0; --i) {
		void* val = queue.delete_min ();
		ASSERT_EQ (val, queue_std.top ().value);
		queue_std.pop ();
	}
}

TYPED_TEST (TestPriorityQueue, Equal)
{
	TypeParam queue;
	RandomGen rndgen;

	static const int MAX_COUNT = 10;
	for (int i = 1; i <= MAX_COUNT; ++i) {
		queue.insert (1, (void*)(intptr_t)(i * PTR_ALIGN), rndgen);
	}

	for (int i = 1; i <= MAX_COUNT; ++i) {
		void* val = queue.delete_min ();
		ASSERT_EQ (val, (void*)(intptr_t)(i * PTR_ALIGN));
	}
}

template <class PQ>
class ThreadTest
{
	static const size_t NUM_PRIORITIES = 20;
	static const int NUM_ITERATIONS = 10000;
	static const int MAX_QUEUE_SIZE = 10000;
public:
	ThreadTest () :
		distr_ (0, NUM_PRIORITIES - 1),
		queue_size_ (0),
		queue_ ()
	{
		for (int i = 0; i < NUM_PRIORITIES; ++i)
			counters_ [i] = 0;
	}

	void thread_proc (int rndinit);

	void finalize ();

private:
	array <atomic <int>, NUM_PRIORITIES> counters_;
	const uniform_int_distribution <int> distr_;
	PQ queue_;
	atomic <int> queue_size_;
};

template <class PQ>
void ThreadTest <PQ>::thread_proc (int rndinit)
{
	RandomGen rndgen (rndinit);

	for (int i = NUM_ITERATIONS; i > 0; --i) {
		if (!bernoulli_distribution (min (1., ((double)queue_size_ / (double)MAX_QUEUE_SIZE))) (rndgen)) {
			unsigned deadline = distr_ (rndgen);
			++counters_ [deadline];
			++queue_size_;
			queue_.insert (deadline, (void*)(intptr_t)((deadline + 1) * PTR_ALIGN), rndgen);
		} else {
			void* p = queue_.delete_min ();
			if (p) {
				size_t deadline = (((intptr_t)p - 1) / PTR_ALIGN);
				--counters_ [deadline];
				--queue_size_;
			}
		}
	}
}

template <class PQ>
void ThreadTest <PQ>::finalize ()
{
	while (void* p = queue_.delete_min ()) {
		size_t deadline = (((intptr_t)p - 1) / PTR_ALIGN);
		--counters_ [deadline];
		--queue_size_;
	}

	EXPECT_EQ (queue_size_, 0);
	for (int i = 0; i < NUM_PRIORITIES; ++i)
		ASSERT_EQ (counters_ [i], 0);
}

TYPED_TEST (TestPriorityQueue, MultiThread)
{
	ThreadTest <TypeParam> test;

	const unsigned int thread_cnt = max (thread::hardware_concurrency (), (unsigned)2);
	vector <thread> threads;

	for (unsigned int i = 0; i < thread_cnt; ++i)
		threads.emplace_back (&ThreadTest <TypeParam>::thread_proc, &test, i + 1);

	for (auto p = threads.begin (); p != threads.end (); ++p)
		p->join ();

	test.finalize ();
}

}
