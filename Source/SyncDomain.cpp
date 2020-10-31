// Nirvana project.
// Synchronization domain.

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

void SyncDomain::schedule ()
{
	while (!running_) {
		DeadlineTime min_deadline;
		if (queue_.get_min_deadline (min_deadline)) {
			DeadlineTime prev_deadline = min_deadline_;
			if (prev_deadline == min_deadline || running_)
				break;
			if (min_deadline_.compare_exchange_strong (prev_deadline, min_deadline)) {
				if (!running_)
					Scheduler::schedule (min_deadline, *this, prev_deadline);
				break;
			}
		} else
			break;
	}
}

void SyncDomain::leave ()
{
	assert (Thread::current ().execution_domain () == current_executor_ && running_);
	current_executor_ = nullptr;
	running_ = false;
	schedule ();
}

void SyncDomain::enter (bool ret)
{
	// TODO: Exception is fatal if ret == true.
	Thread::current ().execution_domain ()->schedule (this);
}

void SyncDomain::async_call (Runnable& runnable, DeadlineTime deadline, CORBA::Nirvana::Interface_ptr environment)
{
	ExecDomain::async_call (runnable, deadline, this, environment);
}

bool SyncDomain::is_free_sync_context ()
{
	return false;
}

Heap& SyncDomain::memory ()
{
	return heap_;
}

}
}
