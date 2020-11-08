// Nirvana project.
// Synchronization domain.

#include "SyncDomain.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

SyncDomain::SyncDomain () :
	scheduler_item_ (Scheduler::create_item (*this)),
	need_schedule_ (false),
	state_ (State::IDLE)
{}

SyncDomain::~SyncDomain ()
{
	Scheduler::release_item (scheduler_item_);
}

void SyncDomain::schedule (QueueNode* node) NIRVANA_NOEXCEPT
{
	verify (queue_.insert (node));
	schedule ();
}

void SyncDomain::schedule () NIRVANA_NOEXCEPT
{
	need_schedule_ = true;
	// Only one thread perform scheduling at a time.
	while (need_schedule_ && State::RUNNING != state_ && !scheduling_.test_and_set ()) {
		need_schedule_ = false;
		DeadlineTime min_deadline;
		if (queue_.get_min_deadline (min_deadline)) {
			if (State::IDLE == state_) {
				state_ = State::SCHEDULED;
				Scheduler::schedule (min_deadline, scheduler_item_);
				scheduled_deadline_ = min_deadline;
			} else if (State::SCHEDULED == state_) {
				DeadlineTime scheduled_deadline = scheduled_deadline_;
				if (
					min_deadline != scheduled_deadline
					&&
					Scheduler::reschedule (min_deadline, scheduler_item_, scheduled_deadline)
					)
						scheduled_deadline_ = min_deadline;
			}
		}
		scheduling_.clear ();
	}
}

void SyncDomain::execute (Word scheduler_error)
{
	assert (State::SCHEDULED == state_);
	state_ = State::RUNNING;
	Executor* executor;
	DeadlineTime dt;
	verify (queue_.delete_min (executor, dt));
	executor->execute (scheduler_error);
	assert (State::RUNNING == state_);
	state_ = State::IDLE;
	schedule ();
}

void SyncDomain::enter (bool ret)
{
	Thread::current ().enter_to (this, ret);
}

SyncDomain* SyncDomain::sync_domain ()
{
	return this;
}

Heap& SyncDomain::memory ()
{
	return heap_;
}

}
}
