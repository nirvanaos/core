// Nirvana project.
// Synchronization domain.

#include "ExecDomain.h"
#include "ScheduleCall.h"
#include "Suspend.h"

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

void SyncDomain::execute (Word scheduler_error)
{
	assert (State::SCHEDULED == state_);
	state_ = State::RUNNING;
	Executor* executor;
	verify (queue_.delete_min (executor));
	executor->execute (scheduler_error);

	// activity_begin() was called in schedule (const DeadlineTime& deadline, Executor& executor);
	// So we call activity_end () here for the balance.
	activity_end ();
}

void SyncDomain::activity_end () NIRVANA_NOEXCEPT
{
	if (0 == activity_cnt_.decrement ())
		Scheduler::delete_item ();
}

void SyncDomain::leave () NIRVANA_NOEXCEPT
{
	assert (State::RUNNING == state_);
	state_ = State::IDLE;
	schedule ();
}

void SyncDomain::enter ()
{
	ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	SyncContext* sync_context = exec_domain->sync_context ();
	assert (sync_context);
	if (!sync_context->sync_domain ()) {
		Core_var <SyncDomain> sd = Core_var <SyncDomain>::create <ImplDynamic <SyncDomain>> ();
		sd->state_ = State::RUNNING;
		exec_domain->sync_context (*sd);
	}
}

void SyncDomain::release_queue_node (QueueNode* node) NIRVANA_NOEXCEPT
{
	assert (node);
	assert (node->domain_ == this);
	queue_.deallocate_node (node);
	activity_end ();
}

void SyncDomain::schedule_call (SyncDomain* sync_domain)
{
	ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	assert (&ExecContext::current () == exec_domain);
	exec_domain->ret_qnode_push (*this);

	if (SyncContext::SUSPEND () == sync_domain)
		Suspend::suspend ();
	else {
		try {
			ScheduleCall::schedule_call (sync_domain);
		} catch (...) {
			release_queue_node (exec_domain->ret_qnode_pop ());
			throw;
		}
		check_schedule_error ();
	}
}

void SyncDomain::schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
	exec_domain.sync_context (*this);
	schedule (exec_domain.ret_qnode_pop (), exec_domain.deadline (), exec_domain);
}

SyncDomain* SyncDomain::sync_domain () NIRVANA_NOEXCEPT
{
	return this;
}

Heap& SyncDomain::memory () NIRVANA_NOEXCEPT
{
	return heap_;
}

}
}
