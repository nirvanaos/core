#include "Scheduler.h"
#include "ExecDomain.h"
#include "initterm.h"

namespace Nirvana {
namespace Core {

AtomicCounter <false> Scheduler::activity_cnt_ (0);
std::atomic <Scheduler::State> Scheduler::state_ (State::RUNNING);

void Scheduler::shutdown () NIRVANA_NOEXCEPT
{
	State state = State::RUNNING;
	if (state_.compare_exchange_strong (state, State::SHUTDOWN_STARTED) && !activity_cnt_) {
		activity_cnt_.increment ();
		activity_end ();
	}
}

void Scheduler::activity_begin ()
{
	activity_cnt_.increment ();
	if (State::RUNNING != state_) {
		activity_end ();
		throw CORBA::INITIALIZE ();
	}
}

inline
void Scheduler::check_shutdown () NIRVANA_NOEXCEPT
{
	State state = State::SHUTDOWN_STARTED;
	if (state_.compare_exchange_strong (state, State::SHUTDOWN_FINISH)) {
		terminate ();
		Port::Scheduler::shutdown ();
	}
}

void Scheduler::activity_end () NIRVANA_NOEXCEPT
{
	if (!activity_cnt_.decrement ())
		check_shutdown ();
}

}
}
