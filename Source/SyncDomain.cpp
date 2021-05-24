/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

SyncDomain::SyncDomain () :
	need_schedule_ (false),
	state_ (State::IDLE),
	activity_cnt_ (0)
{
#ifdef _DEBUG
	Thread* cur = Thread::current_ptr ();
	if (cur) {
		ExecDomain* ed = cur->exec_domain ();
		assert (!ed->sync_context ().sync_domain ());
	}
#endif
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

void SyncDomain::execute (int scheduler_error)
{
	assert (State::SCHEDULED == state_);
	state_ = State::RUNNING;
	Executor* executor;
	verify (queue_.delete_min (executor));
	_add_ref ();
	executor->execute (scheduler_error);

	// activity_begin() was called in schedule (const DeadlineTime& deadline, Executor& executor);
	// So we call activity_end () here for the balance.
	activity_end ();
	_remove_ref ();
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

SyncDomain& SyncDomain::enter ()
{
	ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	SyncContext& sync_context = exec_domain->sync_context ();
	SyncDomain* psd = sync_context.sync_domain ();
	if (!psd) {
		CoreRef <SyncDomain> sd = CoreRef <SyncDomain>::create <ImplDynamic <SyncDomain>> ();
		sd->state_ = State::RUNNING;
		exec_domain->sync_context (*sd);
		psd = sd;
	}
	return *psd;
}

void SyncDomain::release_queue_node (QueueNode* node) NIRVANA_NOEXCEPT
{
	assert (node);
	assert (node->domain_ == this);
	queue_.deallocate_node (node);
	activity_end ();
}

void SyncDomain::schedule_call (SyncContext& target)
{
	ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	assert (&ExecContext::current () == exec_domain);
	exec_domain->ret_qnode_push (*this);

	try {
		exec_domain->schedule_call (target);
	} catch (...) {
		release_queue_node (exec_domain->ret_qnode_pop ());
		throw;
	}
	check_schedule_error (*exec_domain);
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

RuntimeSupport& SyncDomain::runtime_support () NIRVANA_NOEXCEPT
{
	return runtime_support_;
}

}
}
