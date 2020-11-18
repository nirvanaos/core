#ifndef NIRVANA_CORE_SCHEDULER_H_
#define NIRVANA_CORE_SCHEDULER_H_

#include <Port/Scheduler.h>
#include "AtomicCounter.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class Scheduler : public Port::Scheduler
{
public:
	static void shutdown () NIRVANA_NOEXCEPT
	{
		State state = State::RUNNING;
		if (state_.compare_exchange_strong (state, State::SHUTDOWN_STARTED) && !activity_cnt_) {
			activity_cnt_.increment ();
			activity_end ();
		}
	}

	static void activity_begin ()
	{
		activity_cnt_.increment ();
		if (State::RUNNING != state_) {
			activity_end ();
			throw CORBA::INITIALIZE ();
		}
	}
	
	static void activity_end () NIRVANA_NOEXCEPT
	{
		if (!activity_cnt_.decrement ())
			do_shutdown ();
	}

private:
	static void do_shutdown () NIRVANA_NOEXCEPT
	{
		State state = State::SHUTDOWN_STARTED;
		if (state_.compare_exchange_strong (state, State::SHUTDOWN_FINISH))
			Port::Scheduler::shutdown ();
	}

private:
	enum class State
	{
		RUNNING = 0,
		SHUTDOWN_STARTED,
		SHUTDOWN_FINISH
	};

	static std::atomic <State> state_;
	static AtomicCounter <false> activity_cnt_;
};

}
}

#endif
