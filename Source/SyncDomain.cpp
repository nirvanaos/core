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

namespace Nirvana {
namespace Core {

SyncDomain::SyncDomain (Ref <MemContext>&& mem_context) noexcept :
	SyncContext (true),
	mem_context_ (std::move (mem_context)),
	queue_ (mem_context_->heap ()),
	state_ (State::IDLE),
	activity_cnt_ (0),
	queue_items_ (0)
#ifndef NDEBUG
	, executing_domain_ (nullptr)
#endif
{
#ifndef NDEBUG
	Thread* cur = Thread::current_ptr ();
	if (cur && cur->executing ()) {
		ExecDomain* ed = cur->exec_domain ();
		assert (!ed->sync_context ().sync_domain ());
	}
#endif
}

SyncDomain::~SyncDomain ()
{
	assert (!activity_cnt_);
	if (activity_cnt_)
		Scheduler::delete_item (true);
}

void SyncDomain::schedule () noexcept
{
	// Only one thread perform scheduling at a given time.
	State state = State::IDLE;
	if (state_.compare_exchange_strong (state, State::SCHEDULING)) {

		if (0 == queue_items_.load ()) {
			scheduling_end ();
			return;
		}

		DeadlineTime min_deadline;
		NIRVANA_VERIFY (queue_.get_min_deadline (min_deadline));
		Scheduler::schedule (min_deadline, *this);
		scheduled_deadline_ = min_deadline;
		if (scheduling_end ())
			return;
		if (queue_items_.load () <= 1)
			return;
	}

	for (;;) {
		state = State::SCHEDULED;
		if (!state_.compare_exchange_strong (state, State::SCHEDULING))
			break;

		auto qitems = queue_items_.load ();
		DeadlineTime min_deadline;
		NIRVANA_VERIFY (queue_.get_min_deadline (min_deadline));
		if (min_deadline < scheduled_deadline_) {
			if (!Scheduler::reschedule (min_deadline, *this, scheduled_deadline_)) {
				state_.store (State::SCHEDULING_END, std::memory_order_release);
				return;
			}
			scheduled_deadline_ = min_deadline;
		}
		if (scheduling_end ())
			return;
		if (queue_items_.load () <= qitems)
			break;
	}
}

bool SyncDomain::scheduling_end () noexcept
{
	State state = State::SCHEDULING;
	if (!state_.compare_exchange_strong (state, State::SCHEDULED)) {
		assert (state == State::SCHEDULING_STOP);
		state_.store (State::SCHEDULING_END, std::memory_order_release);
		return true;
	}
	return false;
}

void SyncDomain::execute (Thread& worker, Ref <Executor> holder) noexcept
{
	assert (State::IDLE != state_);
	assert (queue_items_ > 0);

	Ref <Executor> executor;
	{
		Port::Thread::PriorityBoost boost (&worker);

		for (BackOff bo;;) {
			State state = State::SCHEDULED;
			if (state_.compare_exchange_strong (state, State::EXECUTING))
				break;
			state = State::SCHEDULING;
			state_.compare_exchange_strong (state, State::SCHEDULING_STOP);
			state = State::SCHEDULING_END;
			if (state_.compare_exchange_weak (state, State::EXECUTING))
				break;
			bo ();
		}

		assert (State::EXECUTING == state_);
		queue_items_.decrement ();

		NIRVANA_VERIFY (queue_.delete_min (executor));
	}

	ExecDomain& ed = static_cast <ExecDomain&> (*executor);
#ifndef NDEBUG
	executing_domain_ = &ed;
#endif
	ed.execute (worker, std::move (executor), Ref <SyncDomain>::cast (std::move (holder)));
}

void SyncDomain::leave () noexcept
{
#ifndef NDEBUG
	assert (executing_domain_);
	assert (executing_domain_ == &ExecDomain::current ());
	assert (State::EXECUTING == state_);
	executing_domain_ = nullptr;
#endif

	// activity_begin() was called in schedule (const DeadlineTime& deadline, Executor& executor);
	// So we call activity_end () here for the balance.
	activity_end ();

	// Check queue and enter the State::IDLE
	bool sched = queue_items_.load () > 0;
	state_.store (State::IDLE, std::memory_order_relaxed);
	if (sched)
		schedule ();
}

SyncDomain& SyncDomain::enter ()
{
	ExecDomain& exec_domain = ExecDomain::current ();
	SyncContext& sync_context = exec_domain.sync_context ();
	SyncDomain* psd = sync_context.sync_domain ();
	if (!psd) {
		
		if (!sync_context.is_free_sync_context ())
			throw_NO_PERMISSION (); // Process called value factory with interface support

		switch (exec_domain.sync_context ().sync_context_type ()) {
		case SyncContext::Type::FREE_MODULE_INIT:
		case SyncContext::Type::FREE_MODULE_TERM:
			// Must not create sync domain in read-only heap
		case SyncContext::Type::SINGLETON_TERM:
			// Must not create sync domain in singleton termination code
			throw_NO_PERMISSION ();
		}

		MemContext& mc = exec_domain.mem_context ();
		Ref <SyncDomain> sd = SyncDomainDyn <SyncDomainUser>::create (mc.heap (),
			std::ref (sync_context), std::ref (mc));
		sd->activity_begin ();
		sd->state_ = State::EXECUTING;
		exec_domain.sync_context (*sd);
#ifndef NDEBUG
		sd->executing_domain_ = &exec_domain;
#endif
		exec_domain.on_enter_sync_domain (*sd);
		psd = sd;
	}
	return *psd;
}

void SyncDomain::activity_begin ()
{
	if (1 == activity_cnt_.increment_seq ())
		Scheduler::create_item (true);
}

void SyncDomain::activity_end () noexcept
{
	if (0 == activity_cnt_.decrement_seq ())
		Scheduler::delete_item (true);
}

void SyncDomain::release_queue_node (QueueNode* node) noexcept
{
	assert (node);
	assert (node->domain_ == this);
	queue_.deallocate_node (node);
	activity_end ();
}

SyncContext::Type SyncDomain::sync_context_type () const noexcept
{
	return SyncContext::Type::SYNC_DOMAIN;
}

Module* SyncDomainUser::module () noexcept
{
	return parent_->module ();
}

SyncDomainCore::SyncDomainCore (Heap& heap) :
	SyncDomain (MemContext::create (heap, true))
{}

Module* SyncDomainCore::module () noexcept
{
	return nullptr;
}

}
}
