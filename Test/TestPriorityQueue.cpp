#include "../Source/PriorityQueue.h"
#include <Mock/MockMemory.h>
#include <gtest/gtest.h>
#include <queue>
#include <array>
#include <thread>
#include <random>
#include <vector>
#include <atomic>

using namespace std;
using ::Nirvana::DeadlineTime;
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
		::Nirvana::Core::g_core_heap = ::Nirvana::Test::mock_memory ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}
};

struct Value
{
	int idx;
	unsigned deadline;

	bool operator < (const Value& rhs) const
	{
		return idx < rhs.idx;
	}
};

struct StdNode : Value
{
	bool operator < (const StdNode& rhs) const
	{ // STL priority queue sorted in descending order.
		if (deadline > rhs.deadline)
			return true;
		else if (deadline < rhs.deadline)
			return false;
		return idx > rhs.idx;
	}
};

typedef std::priority_queue <StdNode> StdPriorityQueue;

typedef ::testing::Types <
	PriorityQueue <Value, 1>,
	PriorityQueue <Value, 2>,
	PriorityQueue <Value, 4>,
	PriorityQueue <Value, 16>
> MaxLevel;

TYPED_TEST_CASE (TestPriorityQueue, MaxLevel);

TYPED_TEST (TestPriorityQueue, SingleThread)
{
	TypeParam queue;
	StdPriorityQueue queue_std;
	RandomGen rndgen;
	uniform_int_distribution <int> distr;

	Value v;
	ASSERT_FALSE (queue.delete_min (v));

	static const int MAX_COUNT = 1000;
	for (int i = 0; i < MAX_COUNT; ++i) {
		unsigned deadline = distr (rndgen);
		StdNode node;
		node.deadline = deadline;
		node.idx = i;
		ASSERT_TRUE (queue.insert (deadline, node, rndgen));
		queue_std.push (node);
	}

	for (int i = 0; i < MAX_COUNT; ++i) {
		Value val;
		DeadlineTime deadline;
		ASSERT_TRUE (queue.delete_min (val, deadline));
		ASSERT_EQ (val.deadline, deadline);
		ASSERT_EQ (val.idx, queue_std.top ().idx);
		ASSERT_EQ (val.deadline, queue_std.top ().deadline);
		queue_std.pop ();
	}
}

TYPED_TEST (TestPriorityQueue, Equal)
{
	TypeParam queue;
	RandomGen rndgen;

	{
		Value val = {1, 1};
		ASSERT_TRUE (queue.insert (1, val, rndgen));
		ASSERT_FALSE (queue.insert (1, val, rndgen));
	}

	Value val;
	ASSERT_TRUE (queue.delete_min (val));
	ASSERT_EQ (val.idx, 1);
	ASSERT_EQ (val.deadline, 1);
	ASSERT_FALSE (queue.delete_min (val));
}

TYPED_TEST (TestPriorityQueue, MinMax)
{
	TypeParam queue;
	RandomGen rndgen;

	Value val = {1, 1};
	Value out;
	ASSERT_TRUE (queue.insert (numeric_limits <DeadlineTime>::max (), val, rndgen));
	ASSERT_TRUE (queue.delete_min (out));
	ASSERT_TRUE (queue.insert (0, val, rndgen));
	ASSERT_TRUE (queue.delete_min (out));
}

// Ensure that all values are different.
atomic <int> g_timestamp = 0;

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
			Value val = {g_timestamp++, deadline};
			queue_.insert (deadline, val, rndgen);
		} else {
			Value val;
			DeadlineTime deadline;
			if (queue_.delete_min (val, deadline)) {
				ASSERT_EQ (val.deadline, deadline);
				--counters_ [val.deadline];
				--queue_size_;
			}
		}
	}
}

template <class PQ>
void ThreadTest <PQ>::finalize ()
{
	Value val;
	DeadlineTime deadline;
	while (queue_.delete_min (val, deadline)) {
		ASSERT_EQ (val.deadline, deadline);
		--counters_ [val.deadline];
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

TYPED_TEST (TestPriorityQueue, Erase)
{
	TypeParam queue;
	RandomGen rndgen;

	static const unsigned NUM_ELEMENTS = 100;
	vector <Value> values;
	values.reserve (NUM_ELEMENTS);
	uniform_int_distribution <unsigned> distr;
	for (int i = 0; i < NUM_ELEMENTS; ++i) {
		values.push_back ({i, distr (rndgen)});
	}
	for (auto p = values.begin (); p != values.end (); ++p) {
		ASSERT_TRUE (queue.insert (p->deadline, *p, rndgen));
	}
	for (auto p = values.begin (); p != values.end (); ++p) {
		ASSERT_TRUE (queue.erase (p->deadline, *p));
	}
}

}
