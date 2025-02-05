/// \file
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
#ifndef NIRVANA_CORE_EVENTSYNCTIMEOUT_H_
#define NIRVANA_CORE_EVENTSYNCTIMEOUT_H_
#pragma once

#include "UserAllocator.h"
#include "TimerAsyncCall.h"
#include <forward_list>

namespace Nirvana {
namespace Core {

class ExecDomain;
class Synchronized;

/// \brief Implements wait list for synchronized operations with timeout feature.
class EventSyncTimeout
{
	static const TimeBase::TimeT TIMER_DEADLINE = 1 * TimeBase::MILLISECOND;

public:
	EventSyncTimeout ();

	bool wait (TimeBase::TimeT timeout, Synchronized* frame = nullptr);

	void signal_all () noexcept;
	void signal_one () noexcept;

	void reset_one () noexcept
	{
		assert (signal_cnt_ && std::numeric_limits <size_t>::max () != signal_cnt_);
		--signal_cnt_;
	}

	void reset_all () noexcept
	{
		assert (list_.empty ());
		signal_cnt_ = 0;
	}

	size_t signal_cnt () const noexcept
	{
		return signal_cnt_;
	}

	static TimeBase::TimeT ms2time (uint32_t ms) noexcept
	{
		switch (ms) {
		case 0:
			return 0;

		case std::numeric_limits <uint32_t>::max ():
			return std::numeric_limits <TimeBase::TimeT>::max ();

		default:
			return (TimeBase::TimeT)ms * TimeBase::MILLISECOND;
		}
	}

private:
	void on_timer () noexcept;

	bool acq_signal () noexcept
	{
		if (signal_cnt_) {
			if (std::numeric_limits <size_t>::max () != signal_cnt_)
				--signal_cnt_;
			return true;
		}
		return false;
	}

private:
	struct ListEntry
	{
		ExecDomain* exec_domain;
		TimeBase::TimeT expire_time;
		volatile bool * result_ptr;
	};

	typedef std::forward_list <ListEntry, UserAllocator <ListEntry> > List;

	class Timer : public TimerAsyncCall
	{
	protected:
		Timer (EventSyncTimeout& ev) :
			TimerAsyncCall (SyncContext::current (), TIMER_DEADLINE),
			event_ (ev)
		{}

	private:
		virtual void run (const TimeBase::TimeT& signal_time) override;

	private:
		EventSyncTimeout& event_;
	};

	List list_;
	Ref <Timer> timer_;
	TimeBase::TimeT next_timeout_;
	size_t signal_cnt_;
};

}
}

#endif
