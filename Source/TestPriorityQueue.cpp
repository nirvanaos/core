#include "../Source/PriorityQueue.h"
#include "gtest/gtest.h"
#include <queue>

using namespace ::CORBA;
using namespace ::Nirvana;
using ::Nirvana::Core::PriorityQueue;

namespace TestPriorityQueue {

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
		::Nirvana::Core::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::Core::terminate ();
	}
};

struct StdNode
{
	PriorityQueue::Key key;
	void* value;

	bool operator < (const StdNode& rhs) const
	{
		return key > rhs.key;
	}
};

typedef std::priority_queue <StdNode> StdPriorityQueue;

TEST_F (TestPriorityQueue, Single)
{
	PriorityQueue queue;
	StdPriorityQueue queue_std;
	PriorityQueue::RandomGen rndgen;
	std::uniform_int_distribution <int> distr (1);

	ASSERT_FALSE (queue.delete_min ());

	static const int MAX_COUNT = 1000;
	for (int i = MAX_COUNT; i > 0; --i) {
		int prio = distr (rndgen);
		StdNode node;
		node.key = prio;
		node.value = (void*)(intptr_t)(prio << 1);
		queue.insert (prio, node.value, rndgen);
		queue_std.push (node);
	}

	for (int i = MAX_COUNT; i > 0; --i) {
		void* val = queue.delete_min ();
		ASSERT_EQ (val, queue_std.top ().value);
		queue_std.pop ();
	}
}

TEST_F (TestPriorityQueue, Equal)
{
	PriorityQueue queue (1);
	PriorityQueue::RandomGen rndgen;

	static const int MAX_COUNT = 2;
	for (int i = 1; i <= MAX_COUNT; --i) {
		queue.insert (1, (void*)(intptr_t)(i * 2), rndgen);
	}

	for (int i = 1; i <= MAX_COUNT; --i) {
		void* val = queue.delete_min ();
		ASSERT_EQ (val, (void*)(intptr_t)(i * 2));
	}
}

}
