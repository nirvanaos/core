#include "pch.h"
#include "../Source/PriorityQueueReorder.h"
#include "../Source/SkipListWithPool.h"
#include "../Source/MemContextCore.h"
#include <queue>
#include <array>
#include <thread>
#include <random>
#include <vector>
#include <atomic>

using Nirvana::DeadlineTime;
using Nirvana::Core::PriorityQueueReorder;
using Nirvana::Core::SkipListWithPool;
using Nirvana::Core::RandomGen;

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
		ASSERT_TRUE (Nirvana::Core::Heap::initialize ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Nirvana::Core::Heap::terminate ();
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

typedef SkipListWithPool <PriorityQueueReorder <Value, 10> > QueueWithPoolBase;

class QueueWithPool : public QueueWithPoolBase
{
public:
	QueueWithPool () :
		QueueWithPoolBase (8)
	{}
};

typedef ::testing::Types <
	PriorityQueueReorder <Value, 10> // 0
	// , QueueWithPool              // 1
> QTypes;

TYPED_TEST_SUITE (TestPriorityQueue, QTypes);

TYPED_TEST (TestPriorityQueue, SingleThread)
{
	TypeParam queue;
	StdPriorityQueue queue_std;
	RandomGen rndgen;
	std::uniform_int_distribution <int> distr;

	Value v;
	ASSERT_FALSE (queue.delete_min (v));
	EXPECT_TRUE (queue.empty ());

	static const int MAX_COUNT = 1000;
	for (int i = 0; i < MAX_COUNT; ++i) {
		unsigned deadline = distr (rndgen);
		StdNode node;
		node.deadline = deadline;
		node.idx = i;
		ASSERT_TRUE (queue.insert (deadline, node));
		EXPECT_FALSE (queue.empty ());
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

	EXPECT_TRUE (queue.empty ());
}

TYPED_TEST (TestPriorityQueue, Reorder)
{
	TypeParam queue;
	StdPriorityQueue queue_std;
	RandomGen rndgen;
	std::uniform_int_distribution <int> distr;

	static const int MAX_COUNT = 1000;
	for (int i = 0; i < MAX_COUNT; ++i) {
		unsigned deadline = distr (rndgen);
		StdNode node;
		node.deadline = deadline;
		node.idx = i;
		ASSERT_TRUE (queue.insert (deadline, node));
		EXPECT_FALSE (queue.empty ());
		queue_std.push (node);
	}

	for (int j = 0; j < 2; ++j) {
		StdPriorityQueue tmp;
		tmp.swap (queue_std);
		for (int i = 0; i < MAX_COUNT; ++i) {
			unsigned deadline = distr (rndgen);
			StdNode val = tmp.top ();
			if (deadline != val.deadline) {
				DeadlineTime old = val.deadline;
				val.deadline = deadline;
				ASSERT_TRUE (queue.reorder (deadline, val, old));
			}
			queue_std.push (val);
			tmp.pop ();
		}
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

	EXPECT_TRUE (queue.empty ());
}

// Ensure that all values are different.
std::atomic <int> g_timestamp (0);

template <class PQ>
class ThreadTest
{
	static const size_t NUM_PRIORITIES = 20;
	static const int NUM_ITERATIONS = 500000;
	static const int MAX_QUEUE_SIZE = 10000;

public:
	ThreadTest () :
		queue_ (),
		queue_size_ (0)
	{
		for (int i = 0; i < NUM_PRIORITIES; ++i)
			counters_ [i] = 0;
	}

	void thread_proc ();

	void finalize ();

private:
	std::array <std::atomic <int>, NUM_PRIORITIES> counters_;
	PQ queue_;
	std::atomic <int> queue_size_;
};

template <class PQ>
void ThreadTest <PQ>::thread_proc ()
{
	RandomGen rndgen;
	std::uniform_int_distribution <int> distr (0, NUM_PRIORITIES - 1);

	std::vector <Value> values;

	static const bool REORDER = true;

	for (int i = NUM_ITERATIONS; i > 0; --i) {
		if (!std::bernoulli_distribution (std::min (1., ((double)queue_size_ / (double)MAX_QUEUE_SIZE))) (rndgen)) {
			unsigned deadline = distr (rndgen);
			++counters_ [deadline];
			++queue_size_;
			Value val = {g_timestamp++, deadline};
			ASSERT_TRUE (queue_.insert (deadline, val));
			values.push_back (val);
		} else if (!REORDER || values.empty () || std::bernoulli_distribution (0.5) (rndgen)) {
			Value val;
			DeadlineTime deadline;
			if (queue_.delete_min (val, deadline)) {
				ASSERT_EQ (val.deadline, deadline);
				--counters_ [val.deadline];
				--queue_size_;
			}
		} else {
			size_t idx = std::uniform_int_distribution <size_t> (0, values.size () - 1) (rndgen);
			Value& val = values [idx];
			unsigned old = val.deadline;
			unsigned deadline;
			for (;;) {
				deadline = distr (rndgen);
				if (deadline != old)
					break;
			}
			val.deadline = deadline;
			if (queue_.reorder (deadline, val, old)) {
				--counters_ [old];
				++counters_ [deadline];
			} else
				values.erase (values.begin () + idx);
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

	const unsigned int thread_cnt = std::thread::hardware_concurrency ();
	std::vector <std::thread> threads;

	for (unsigned int i = 0; i < thread_cnt; ++i)
		threads.emplace_back (&ThreadTest <TypeParam>::thread_proc, &test);

	for (auto p = threads.begin (); p != threads.end (); ++p)
		p->join ();

	test.finalize ();
}

}
