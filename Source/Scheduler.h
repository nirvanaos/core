#ifndef NIRVANA_CORE_SCHEDULER_H_
#define NIRVANA_CORE_SCHEDULER_H_

#include <Port/Scheduler.h>
#include "AtomicCounter.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class Scheduler
{
public:
	static void schedule (DeadlineTime deadline, Executor& executor, DeadlineTime old, bool nothrow_fallback)
	{
		Port::Scheduler::schedule (deadline, executor, old, nothrow_fallback);
	}

	static void core_free ()
	{
		Port::Scheduler::core_free ();
	}

	static void shutdown ()
	{
		State state = RUNNING;
		if (state_.compare_exchange_strong (state, SHUTDOWN_STARTED) && !activity_cnt_) {
			activity_cnt_.increment ();
			activity_end ();
		}
	}

	static void activity_begin ()
	{
		activity_cnt_.increment ();
		if (RUNNING != state_) {
			activity_end ();
			throw CORBA::INITIALIZE ();
		}
	}
	
	static void activity_end ()
	{
		if (!activity_cnt_.decrement ())
			do_shutdown ();
	}

private:
	static void do_shutdown ()
	{
		State state = SHUTDOWN_STARTED;
		if (state_.compare_exchange_strong (state, SHUTDOWN_FINISH))
			Port::Scheduler::shutdown ();
	}

private:
	enum State
	{
		RUNNING = 0,
		SHUTDOWN_STARTED,
		SHUTDOWN_FINISH
	};

	static std::atomic <State> state_;
	static AtomicCounter activity_cnt_;
};

}
}

#endif
