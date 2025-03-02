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
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

EventSyncTimeout::EventSyncTimeout () :
	next_timeout_ (std::numeric_limits <TimeBase::TimeT>::max ()),
	signal_cnt_ (0)
{}

bool EventSyncTimeout::wait (TimeBase::TimeT timeout, Synchronized* frame)
{
	assert (SyncContext::current ().sync_domain ());

	if (acq_signal ())
		return true;
		
	if (!timeout || Scheduler::shutdown_started ())
		return false;

	volatile bool result = false;

	ExecDomain& cur_ed = frame ? frame->exec_domain () : ExecDomain::current ();

	ListEntry entry{ &cur_ed, std::numeric_limits <TimeBase::TimeT>::max (), &result };
	if (timeout != std::numeric_limits <TimeBase::TimeT>::max ()) {
		TimeBase::TimeT cur_time = Chrono::steady_clock ();
		if (std::numeric_limits <TimeBase::TimeT>::max () - cur_time <= timeout)
			throw_BAD_PARAM ();
		entry.expire_time = cur_time + timeout;
		if (!timer_)
			timer_ = Ref <Timer>::create <ImplDynamic <Timer> > (std::ref (*this));
		if (next_timeout_ > entry.expire_time) {
			timer_->set (0, timeout, 0);
			next_timeout_ = entry.expire_time;
		}
	}

	list_.push_front (entry);
	try {
		if (frame)
			frame->prepare_suspend_and_return ();
		else
			cur_ed.suspend_prepare ();
	} catch (...) {
		list_.pop_front ();
		throw;
	}

	cur_ed.suspend ();

	return result;
}

void EventSyncTimeout::signal_all () noexcept
{
	assert (SyncContext::current ().sync_domain ());

	assert (!signal_cnt_);
	signal_cnt_ = std::numeric_limits <size_t>::max ();

	if (timer_)
		timer_->cancel ();

	while (!list_.empty ()) {
		ListEntry& entry = list_.front ();
		*entry.result_ptr = true;
		ExecDomain* ed = entry.exec_domain;
		list_.pop_front ();
		ed->resume ();
	}
}

void EventSyncTimeout::signal_one () noexcept
{
	assert (SyncContext::current ().sync_domain ());

	assert (signal_cnt_ != std::numeric_limits <size_t>::max ());

	if (!list_.empty ()) {
		ListEntry& entry = list_.front ();
		*entry.result_ptr = true;
		ExecDomain* ed = entry.exec_domain;
		list_.pop_front ();
		if (list_.empty () && timer_)
			timer_->cancel ();
		ed->resume ();
	} else
		++signal_cnt_;
}

inline
void EventSyncTimeout::on_timer () noexcept
{
	assert (SyncContext::current ().sync_domain ());

	TimeBase::TimeT cur_time = Chrono::steady_clock ();
	TimeBase::TimeT next_timeout = std::numeric_limits <TimeBase::TimeT>::max ();
	for (List::iterator it = list_.before_begin ();;) {
		List::iterator next = it;
		++next;
		if (next == list_.end ())
			break;
		TimeBase::TimeT expire_time = next->expire_time;
		if (expire_time <= cur_time) {
			next->exec_domain->resume ();
			list_.erase_after (it);
		} else {
			if (next_timeout > expire_time)
				next_timeout = expire_time;
			it = next;
		}
	}

	if (next_timeout != std::numeric_limits <TimeBase::TimeT>::max ())
		timer_->set (0, next_timeout - cur_time, 0);
}

void EventSyncTimeout::Timer::run (const TimeBase::TimeT& signal_time)
{
	event_.on_timer ();
}

}
}
