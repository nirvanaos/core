#include "../Source/SpinLock.h"
#include "gtest/gtest.h"
#include <thread>
#include <list>

using namespace ::Nirvana::Core;
using namespace ::std;

namespace TestSpinLock {

class TestSpinLock :
	public ::testing::Test
{
protected:
	TestSpinLock ()
	{}

	virtual ~TestSpinLock ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}
};

class Thread : 
	public thread,
	public SpinLockNode
{
public:
	Thread (SpinLock& spin_lock, unsigned iterations) :
		thread (&Thread::run, this, std::ref (spin_lock), iterations)
	{}

	void run (SpinLock& spin_lock, unsigned iterations)
	{
		while (iterations--) {
			spin_lock.acquire (*this);
			spin_lock.release (*this);
		}
	}
};

TEST_F (TestSpinLock, SingleThread)
{
	SpinLock spin_lock;
	SpinLockNode node;
	spin_lock.acquire (node);
	spin_lock.release (node);
}

TEST_F (TestSpinLock, MultiThread)
{
	static const unsigned thread_cnt = thread::hardware_concurrency ();
	static const unsigned iterations = 1000;
	SpinLock spin_lock;

	list <Thread> threads;
	for (unsigned cnt = thread_cnt; cnt; --cnt) {
		threads.emplace_back (spin_lock, iterations);
	}

	for (auto p = threads.begin (); p != threads.end (); ++p) {
		p->join ();
	}
}

}
