// Nirvana project.
// Synchronization domain.

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

void SyncDomain::schedule (ExecDomain& ed, bool ret)
{
	// TODO: If ret==true, get queue entry from pool.
	queue_.insert (ed.deadline (), &ed);
	try {
		schedule (ret);
	} catch (...) {
		queue_.erase (ed.deadline (), &ed);
		throw;
	}
}
void SyncDomain::schedule (bool ret)
{
	while (!running_) {
		DeadlineTime min_deadline;
		if (queue_.get_min_deadline (min_deadline)) {
			DeadlineTime prev_deadline = min_deadline_;
			if (prev_deadline == min_deadline || running_)
				break;
			if (min_deadline_.compare_exchange_strong (prev_deadline, min_deadline)) {
				if (!running_)
					Scheduler::schedule (min_deadline, *this, prev_deadline, ret || prev_deadline);
				break;
			}
		} else
			break;
	}
}

void SyncDomain::execute (DeadlineTime deadline, Word scheduler_error)
{
	assert (deadline);
	DeadlineTime min_deadline = min_deadline_;
	if (min_deadline && deadline >= min_deadline && min_deadline_.compare_exchange_strong (min_deadline, 0)) {
		bool f = false;
		if (running_.compare_exchange_strong (f, true)) {
			ExecDomain* executor;
			DeadlineTime dt;
			if (queue_.delete_min (executor, dt)) {
				executor->execute (dt, scheduler_error);
			}
			running_ = false;
			schedule (true);
		}
	}
}

void SyncDomain::call_begin ()
{
	// TODO: Allocate queue entry and place it to a pool.
}

void SyncDomain::enter (bool ret)
{
	// TODO: Exception is fatal if ret == true.
	Thread::current ().enter_to (this, ret);
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
