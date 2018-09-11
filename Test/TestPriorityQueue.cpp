#include "../Source/PriorityQueue.h"
#include "MockMemory.h"
#include "MockScheduler.h"
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
		::Nirvana::Core::Test::mock_scheduler_init (true);
		::Nirvana::Core::Test::mock_memory_init ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::Core::Test::mock_memory_term ();
		::Nirvana::Core::Test::mock_scheduler_term ();
	}
};

struct StdNode
{
	unsigned deadline;
	unsigned value;

	bool operator < (const StdNode& rhs) const
	{
		return deadline > rhs.deadline;
	}
};

typedef std::priority_queue <StdNode> StdPriorityQueue;

typedef ::testing::Types <
	PriorityQueue <unsigned, 1>,
	PriorityQueue <unsigned, 2>,
	PriorityQueue <unsigned, 4>,
	PriorityQueue <unsigned, 16>
> MaxLevel;

TYPED_TEST_CASE (TestPriorityQueue, MaxLevel);

TYPED_TEST (TestPriorityQueue, SingleThread)
{
	TypeParam queue;
	StdPriorityQueue queue_std;
	RandomGen rndgen;
	uniform_int_distribution <int> distr;

	unsigned d;
	ASSERT_FALSE (queue.delete_min (d));

	static const int MAX_COUNT = 1000;
	for (int i = MAX_COUNT; i > 0; --i) {
		unsigned deadline = distr (rndgen);
		StdNode node;
		node.deadline = deadline;
		node.value = deadline;
		queue.insert (deadline, node.value, rndgen);
		queue_std.push (node);
	}

	for (int i = MAX_COUNT; i > 0; --i) {
		unsigned val;
		ASSERT_TRUE (queue.delete_min (val));
		ASSERT_EQ (val, queue_std.top ().value);
		queue_std.pop ();
	}
}

TYPED_TEST (TestPriorityQueue, Equal)
{
	TypeParam queue;
	RandomGen rndgen;

	static const unsigned MAX_COUNT = 10;
	for (unsigned i = 1; i <= MAX_COUNT; ++i) {
		queue.insert (1, i, rndgen);
	}

	for (unsigned i = 1; i <= MAX_COUNT; ++i) {
		unsigned val;
		ASSERT_TRUE (queue.delete_min (val));
		ASSERT_EQ (val, i);
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

	void thread_proc ();

	void finalize ();

private:
	array <atomic <int>, NUM_PRIORITIES> counters_;
	const uniform_int_distribution <int> distr_;
	PQ queue_;
	atomic <int> queue_size_;
};

template <class PQ>
void ThreadTest <PQ>::thread_proc ()
{
	RandomGen rndgen;

	for (int i = NUM_ITERATIONS; i > 0; --i) {
		if (!bernoulli_distribution (min (1., ((double)queue_size_ / (double)MAX_QUEUE_SIZE))) (rndgen)) {
			unsigned deadline = distr_ (rndgen);
			++counters_ [deadline];
			++queue_size_;
			queue_.insert (deadline, deadline, rndgen);
		} else {
			unsigned deadline;
			if (queue_.delete_min (deadline)) {
				--counters_ [deadline];
				--queue_size_;
			}
		}
	}
}

template <class PQ>
void ThreadTest <PQ>::finalize ()
{
	unsigned deadline;
	while (queue_.delete_min (deadline)) {
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
		threads.emplace_back (&ThreadTest <TypeParam>::thread_proc, &test);

	for (auto p = threads.begin (); p != threads.end (); ++p)
		p->join ();

	test.finalize ();
}

}
