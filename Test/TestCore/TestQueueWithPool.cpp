#include "../Source/MemContextCore.h"
#include "../Source/PriorityQueue.h"
#include "../Source/SkipListWithPool.h"
#include <queue>
#include <random>
#include <gtest/gtest.h>

using Nirvana::DeadlineTime;
using Nirvana::Core::PriorityQueue;
using Nirvana::Core::RandomGen;
using Nirvana::Core::SkipListWithPool;

namespace TestQueueWithPool {

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

typedef SkipListWithPool <PriorityQueue <Value, 10> > Queue;

class TestQueueWithPool :
	public ::testing::Test
{
protected:
	TestQueueWithPool ()
	{}

	virtual ~TestQueueWithPool ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		Nirvana::Core::MemContext::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Nirvana::Core::MemContext::terminate ();
	}
};

TEST_F (TestQueueWithPool, SingleThread)
{
	static const int MAX_COUNT = 1000;
	Queue queue (MAX_COUNT);
	StdPriorityQueue queue_std;
	RandomGen rndgen;
	std::uniform_int_distribution <int> distr;

	Value v;
	ASSERT_FALSE (queue.delete_min (v));

	for (int i = 0; i < MAX_COUNT; ++i) {
		unsigned deadline = distr (rndgen);
		StdNode node;
		node.deadline = deadline;
		node.idx = i;
		ASSERT_TRUE (queue.insert (deadline, node));
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

}
