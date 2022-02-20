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

using namespace std;

namespace Nirvana {
namespace Core {

SyncDomain::SyncDomain (MemContext& memory) NIRVANA_NOEXCEPT :
	mem_context_ (&memory),
	need_schedule_ (false),
	state_ (State::IDLE),
	scheduled_deadline_ (0),
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
	ExecDomain& exec_domain = ExecDomain::current ();
	SyncContext& sync_context = exec_domain.sync_context ();
	SyncDomain* psd = sync_context.sync_domain ();
	if (!psd) {
		CoreRef <SyncDomain> sd = CoreRef <SyncDomain>::create
			<ImplDynamic <SyncDomainImpl>> (ref (sync_context),
				ref (exec_domain.mem_context ()));
		sd->state_ = State::RUNNING;
		exec_domain.sync_context (*sd);
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

SyncDomain* SyncDomain::sync_domain () NIRVANA_NOEXCEPT
{
	return this;
}

Binary* SyncDomainImpl::binary () NIRVANA_NOEXCEPT
{
	return parent_->binary ();
}

void SyncDomainImpl::raise_exception (CORBA::SystemException::Code code, unsigned minor)
{
	parent_->raise_exception (code, minor);
}

}
}
