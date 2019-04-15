#include "../Source/SysScheduler.h"
#include "MockMemory.h"
#include "gtest/gtest.h"
#include <Nirvana/Runnable_s.h>
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
		::Nirvana::Core::Test::mock_memory_init ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::Core::Test::mock_memory_term ();
	}
};

template <class S>
class RunnableImpl :
	public ::CORBA::Nirvana::Servant <S, ::Nirvana::Runnable>,
	public ::CORBA::Nirvana::LifeCycleRefCntAbstract <S>
{};

class SimpleShutdown :
	public RunnableImpl <SimpleShutdown>
{
public:
	~SimpleShutdown ()
	{}

	void run ()
	{
		::Nirvana::Core::SysScheduler::shutdown ();
	}
};

TEST_F (TestScheduler, Startup)
{
	::Nirvana::Runnable_ptr startup (new SimpleShutdown ());
	::Nirvana::Core::Port::Scheduler::run (startup, numeric_limits <Nirvana::DeadlineTime>::max ());
	::CORBA::release (startup);
}

}
