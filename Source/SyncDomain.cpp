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
#include "pch.h"
#include "ExecDomain.h"
#include "MemContextCore.h"

namespace Nirvana {
namespace Core {

SyncDomain::SyncDomain (Ref <MemContext>&& mem_context) noexcept :
	SyncContext (true),
	mem_context_ (std::move (mem_context)),
	scheduling_ ATOMIC_FLAG_INIT,
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

void SyncDomain::schedule () noexcept
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

void SyncDomain::execute () noexcept
{
	assert (State::SCHEDULED == state_);
	state_ = State::RUNNING;
	Executor* executor;
	verify (queue_.delete_min (executor));
	executor->execute ();
}

void SyncDomain::activity_begin ()
{
	if (1 == activity_cnt_.increment_seq ())
		Scheduler::create_item ();
}

void SyncDomain::activity_end () noexcept
{
	if (0 == activity_cnt_.decrement_seq ())
		Scheduler::delete_item ();
}

void SyncDomain::leave () noexcept
{
	assert (State::RUNNING == state_);
	// activity_begin() was called in schedule (const DeadlineTime& deadline, Executor& executor);
	// So we call activity_end () here for the balance.
	activity_end ();
	state_ = State::IDLE;
	schedule ();
}

SyncDomain& SyncDomain::enter ()
{
	ExecDomain& exec_domain = ExecDomain::current ();
	SyncContext& sync_context = exec_domain.sync_context ();
	SyncDomain* psd = sync_context.sync_domain ();
	if (!psd) {
		if (!sync_context.is_free_sync_context ())
			throw_NO_PERMISSION (); // Legacy process called value factory with interface support
		Ref <SyncDomain> sd = Ref <SyncDomain>::create
			<ImplDynamic <SyncDomainImpl> > (std::ref (sync_context),
				std::ref (exec_domain.mem_context ()));
		sd->activity_begin ();
		sd->state_ = State::RUNNING;
		exec_domain.sync_context (*sd);
		psd = sd;
	}
	return *psd;
}

void SyncDomain::release_queue_node (QueueNode* node) noexcept
{
	assert (node);
	assert (node->domain_ == this);
	queue_.deallocate_node (node);
	activity_end ();
}

Module* SyncDomainImpl::module () noexcept
{
	return parent_->module ();
}

void SyncDomainImpl::raise_exception (CORBA::SystemException::Code code, unsigned minor)
{
	parent_->raise_exception (code, minor);
}

SyncDomainCore::SyncDomainCore (Heap& heap) :
	SyncDomain (MemContextCore::create (&heap))
{}

Module* SyncDomainCore::module () noexcept
{
	return nullptr;
}

void SyncDomainCore::raise_exception (CORBA::SystemException::Code code, unsigned minor)
{
	CORBA::SystemException::_raise_by_code (code, minor);
}

}
}
