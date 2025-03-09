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

EventSyncTimeout::EventSyncTimeout (SignalCount initial_cnt) :
	list_ (nullptr),
	signal_cnt_ (initial_cnt)
{}

EventSyncTimeout::~EventSyncTimeout ()
{
	on_exception (CORBA::OBJECT_NOT_EXIST ());
}

bool EventSyncTimeout::wait (TimeBase::TimeT timeout, Synchronized* frame)
{
	assert (SyncContext::current ().sync_domain ());

	if (signal_cnt_) {
		if (SIGNAL_ALL != signal_cnt_)
			--signal_cnt_;
		return true;
	}

	if (!timeout)
		return false;

	if (Scheduler::shutdown_started ()) {
		if (std::numeric_limits <TimeBase::TimeT>::max () == timeout)
			throw_BAD_INV_ORDER ();
		else
			return false;
	}

	ExecDomain& cur_ed = frame ? frame->exec_domain () : ExecDomain::current ();
	DeadlineTime deadline = cur_ed.deadline ();

	ListEntry entry { NO_EXPIRE, cur_ed, nullptr, false };
	if (timeout != std::numeric_limits <TimeBase::TimeT>::max ()) {
		TimeBase::TimeT cur_time = Chrono::steady_clock ();
		if (std::numeric_limits <TimeBase::TimeT>::max () - cur_time <= timeout)
			throw_BAD_PARAM ();
		entry.expire_time = cur_time + timeout;
	}

	ListEntry** link = &list_;
	for (;;) {
		ListEntry* next = *link;
		if (!next || next->exec_domain.deadline () > deadline)
			break;
		link = &(next->next);
	}

	if (entry.expire_time < nearest_expire_time ())
		nearest_expire_time (entry.expire_time);

	if (frame)
		frame->prepare_suspend_and_return ();
	else
		cur_ed.suspend_prepare ();

	entry.next = *link;
	*link = &entry;

	cur_ed.suspend ();

	if (entry.result)
		return true;
	else if (std::numeric_limits <TimeBase::TimeT>::max () == entry.expire_time)
		throw_BAD_INV_ORDER ();
	return false;
}

void EventSyncTimeout::signal_all () noexcept
{
	assert (SyncContext::current ().sync_domain ());

	assert (!signal_cnt_);
	signal_cnt_ = SIGNAL_ALL;

	if (timer_)
		timer_->cancel ();

	ListEntry* entry = list_;
	list_ = nullptr;
	while (entry) {
		ListEntry* next = entry->next;
		entry->result = true;
		entry->exec_domain.resume ();
		entry = next;
	}
}

void EventSyncTimeout::signal_one () noexcept
{
	assert (SyncContext::current ().sync_domain ());

	assert (signal_cnt_ != SIGNAL_ALL);

	ListEntry* entry = list_;
	if (entry) {
		ListEntry* next = entry->next;
		list_ = next;
		entry->result = true;
		TimeBase::TimeT expire_time = entry->expire_time;
		entry->exec_domain.resume ();
		if (NO_EXPIRE != expire_time && expire_time <= nearest_expire_time ()) {
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

void EventSyncTimeout::on_exception (const CORBA::Exception& exc) noexcept
{
	nearest_expire_time (NO_EXPIRE);

	ListEntry* entry = list_;
	list_ = nullptr;
	while (entry) {
		ListEntry* next = entry->next;
		entry->exec_domain.resume (exc);
		entry = next;
	}
}

}
}
