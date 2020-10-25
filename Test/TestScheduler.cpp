#include "../Source/Scheduler.h"
#include "../Source/Heap.h"
#include "gtest/gtest.h"
#include <limits>

namespace TestScheduler {

using namespace std;
using namespace Nirvana;
using namespace Nirvana::Core;

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
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}
};

class SimpleShutdown :
	public Runnable
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
		ran_ = true;
		Scheduler::shutdown ();
	}

	static bool exists_;
	static bool ran_;
};

bool SimpleShutdown::exists_ = false;
bool SimpleShutdown::ran_ = false;

TEST_F (TestScheduler, Startup)
{
	{
		Core_var <Runnable> startup = Core_var <Runnable>::create <ImplDynamic <SimpleShutdown> > ();
		Port::Scheduler::run_system_domain (*startup, numeric_limits <Nirvana::DeadlineTime>::max ());
		ASSERT_TRUE (SimpleShutdown::exists_);
		ASSERT_TRUE (SimpleShutdown::ran_);
	}
	ASSERT_FALSE (SimpleShutdown::exists_);
}

}
