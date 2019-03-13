// Nirvana project
// Core tests
// MockScheduler class

#include "../Source/core.h"
#include "../Source/Scheduler_s.h"
#include <thread>

namespace Nirvana {
namespace Core {
namespace Test {

class MockScheduler :
	public ::CORBA::Nirvana::ServantStatic <MockScheduler, Scheduler>
{
public:
	static void initialize (bool backoff_break)
	{
		backoff_break_ = backoff_break;
		::Nirvana::Core::g_scheduler = _this ();
	}

	static void terminate ()
	{
		::Nirvana::Core::g_scheduler = Scheduler::_nil ();
	}

	static void backoff_break (bool on)
	{
		backoff_break_ = on;
	}

	static void schedule (DeadlineTime deadline, Executor_ptr runnable, DeadlineTime deadline_prev)
	{
		throw ::CORBA::NO_IMPLEMENT ();
	}

	static void back_off (::CORBA::ULong hint)
	{
		assert (!backoff_break_);
		if (hint)
			std::this_thread::sleep_for (std::chrono::milliseconds (hint));
		else
			std::this_thread::yield ();
	}

	static void core_free ()
	{}

private:
	static bool backoff_break_;
};

bool MockScheduler::backoff_break_ = false;

void mock_scheduler_init (bool backoff_break)
{
	MockScheduler::initialize (backoff_break);
}

void mock_scheduler_term ()
{
	MockScheduler::terminate ();
}

void mock_scheduler_backoff_break (bool on)
{
	MockScheduler::backoff_break (on);
}

}
}
}

