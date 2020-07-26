#include "../Source/Scheduler.h"
#include "../Source/Heap.h"
#include "gtest/gtest.h"
#include <limits>

namespace TestScheduler {

using namespace std;

class TestScheduler :
	public ::testing::Test
{
protected:
	TestScheduler ()
	{}

	virtual ~TestScheduler ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		::Nirvana::Core::Heap::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::Core::Heap::terminate ();
	}
};

class SimpleShutdown :
	public Nirvana::Core::ImplDynamic <SimpleShutdown, Nirvana::Core::Runnable>
{
public:
	SimpleShutdown ()
	{
		exists_ = true;
	}

	~SimpleShutdown ()
	{
		exists_ = false;
	}

	void run ()
	{
		::Nirvana::Core::Scheduler::shutdown ();
	}

	static bool exists_;
};

bool SimpleShutdown::exists_ = false;

TEST_F (TestScheduler, Startup)
{
	{
		Nirvana::Core::Core_var <Nirvana::Core::Runnable> startup (new SimpleShutdown ());
		Nirvana::Core::Port::Scheduler::run_system_domain (*startup, numeric_limits <Nirvana::DeadlineTime>::max ());
		ASSERT_TRUE (SimpleShutdown::exists_);
	}
	ASSERT_FALSE (SimpleShutdown::exists_);
}

}
