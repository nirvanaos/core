// Nirvana project.
// Synchronization domain.

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

SyncDomain::SyncDomain () :
	need_schedule_ (false),
	state_ (State::IDLE),
	activity_cnt_ (0)
{
}

SyncDomain::~SyncDomain ()
{
	assert (!activity_cnt_);
	if (activity_cnt_)
		Scheduler::delete_item ();
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
				Scheduler::schedule (min_deadline, *this);
				scheduled_deadline_ = min_deadline;
			} else if (State::SCHEDULED == state_) {
				DeadlineTime scheduled_deadline = scheduled_deadline_;
				if (
					min_deadline != scheduled_deadline
					&&
					Scheduler::reschedule (min_deadline, *this, scheduled_deadline)
					)
						scheduled_deadline_ = min_deadline;
			}
		}
		scheduling_.clear ();
	}
}

inline
void SyncDomain::activity_end () NIRVANA_NOEXCEPT
{
	if (1 == activity_cnt_.decrement ())
		Scheduler::delete_item ();
}

SyncDomain::QueueNode* SyncDomain::create_queue_node (QueueNode* next)
{
	assert (0 != activity_cnt_);
	QueueNode* node = static_cast <QueueNode*> (queue_.allocate_node ());
	activity_cnt_.increment ();
	node->domain_ = this;
	node->next_ = next;
	return node;
}

void SyncDomain::release_queue_node (QueueNode* node) NIRVANA_NOEXCEPT
{
	assert (node);
	assert (node->domain_ == this);
	queue_.deallocate_node (node);
	activity_end ();
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
	activity_end ();
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
