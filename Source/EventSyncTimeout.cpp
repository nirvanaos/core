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
#include "EventSyncTimeout.h"
#include "Chrono.h"
#include "Synchronized.h"

namespace Nirvana {
namespace Core {

EventSyncTimeout::EventSyncTimeout () :
	list_ (nullptr),
	nearest_expire_time_ (0),
	signal_cnt_ (0)
{}

EventSyncTimeout::~EventSyncTimeout ()
{
	if (timer_)
		timer_->disconnect ();
	CORBA::OBJECT_NOT_EXIST exc;
	resume_all (&exc);
}

bool EventSyncTimeout::wait (TimeBase::TimeT timeout, Synchronized* frame)
{
	assert (SyncContext::current ().sync_domain ());

	if (acq_signal ())
		return true;
		
	if (!timeout)
		return false;

	if (Scheduler::shutdown_started ()) {
		if (timeout != std::numeric_limits <TimeBase::TimeT>::max ())
			return false;
		else
			throw_BAD_INV_ORDER ();
	}

	ExecDomain& cur_ed = frame ? frame->exec_domain () : ExecDomain::current ();
	DeadlineTime deadline = cur_ed.deadline ();

	ListEntry entry { std::numeric_limits <TimeBase::TimeT>::max (), &cur_ed, nullptr, false };
	if (timeout != std::numeric_limits <TimeBase::TimeT>::max ()) {
		TimeBase::TimeT cur_time = Chrono::steady_clock ();
		if (std::numeric_limits <TimeBase::TimeT>::max () - cur_time <= timeout)
			throw_BAD_PARAM ();
		entry.expire_time = cur_time + timeout;
	}

	ListEntry* prev = nullptr;
	for (ListEntry* next = list_; next; next = next->next) {
		if (next->exec_domain->deadline () > deadline)
			break;
		prev = next;
	}

	if (!timer_)
		timer_ = Ref <Timer>::create <ImplDynamic <Timer> > (std::ref (*this));

	if (frame)
		frame->prepare_suspend_and_return ();
	else
		cur_ed.suspend_prepare ();

	if (prev) {
		entry.next = prev->next;
		prev->next = &entry;
	} else {
		entry.next = list_;
		list_ = &entry;
	}

	if (0 == nearest_expire_time_ || nearest_expire_time_ > entry.expire_time) {
		nearest_expire_time_ = entry.expire_time;
		TimeBase::TimeT timeout = std::min (entry.expire_time - Chrono::steady_clock (), CHECK_TIMEOUT);
		timer_->set (0, timeout, 0);
	}

	cur_ed.suspend ();

	if (entry.result)
		return true;
	else if (std::numeric_limits <TimeBase::TimeT>::max () == entry.expire_time)
		throw_BAD_INV_ORDER ();
	return false;
}

void EventSyncTimeout::nearest_expire_time (TimeBase::TimeT t) noexcept
{
	if (timer_ && nearest_expire_time_ != t) {
		if (!list_) {
			nearest_expire_time_ = 0;
			timer_->cancel ();
		} else {
			nearest_expire_time_ = t;
			TimeBase::TimeT timeout = std::min (t - Chrono::steady_clock (), CHECK_TIMEOUT);
			try {
				timer_->set (0, timeout, 0);
			} catch (const CORBA::Exception& exc) {
				resume_all (&exc);
			}
		}
	}
}

void EventSyncTimeout::signal_all () noexcept
{
	assert (SyncContext::current ().sync_domain ());

	assert (!signal_cnt_);
	signal_cnt_ = std::numeric_limits <size_t>::max ();

	if (timer_)
		timer_->cancel ();

	while (list_) {
		list_->result = true;
		ExecDomain* ed = list_->exec_domain;
		list_ = list_->next;
		ed->resume ();
	}
}

void EventSyncTimeout::signal_one () noexcept
{
	assert (SyncContext::current ().sync_domain ());

	assert (signal_cnt_ != std::numeric_limits <size_t>::max ());

	if (list_) {
		list_->result = true;
		ExecDomain* ed = list_->exec_domain;
		TimeBase::TimeT expire_time = list_->expire_time;
		list_ = list_->next;
		ed->resume ();
		if (timer_ && std::numeric_limits <TimeBase::TimeT>::max () != expire_time
			&& expire_time <= nearest_expire_time_) {
			expire_time = std::numeric_limits <TimeBase::TimeT>::max ();
			for (ListEntry* entry = list_; entry; entry = entry->next) {
				if (expire_time > entry->expire_time)
					expire_time = entry->expire_time;
			}
			nearest_expire_time (expire_time);
		}
	} else
		++signal_cnt_;
}

inline
void EventSyncTimeout::on_timer () noexcept
{
	assert (SyncContext::current ().sync_domain ());

	if (Scheduler::shutdown_started ())
		resume_all (nullptr);
	else {
		TimeBase::TimeT cur_time = Chrono::steady_clock ();
		TimeBase::TimeT next_time = std::numeric_limits <TimeBase::TimeT>::max ();
		for (ListEntry* entry = list_, *prev = nullptr; entry;) {
			if (entry->expire_time >= cur_time) {
				if (prev)
					prev->next = entry->next;
				else
					list_ = entry->next;
				entry->exec_domain->resume ();
			} else {
				if (next_time > entry->expire_time)
					next_time = entry->expire_time;
				prev = entry;
				entry = entry->next;
			}
		}

		nearest_expire_time (next_time);
	}
}

void EventSyncTimeout::resume_all (const CORBA::Exception* exc) noexcept
{
	if (timer_)
		timer_->cancel ();

	while (list_) {
		list_->result = true;
		ExecDomain* ed = list_->exec_domain;
		list_ = list_->next;
		if (exc)
			ed->resume (*exc);
		else
			ed->resume ();
	}
}

void EventSyncTimeout::Timer::run (const TimeBase::TimeT& signal_time)
{
	if (event_)
		event_->on_timer ();
}

}
}
