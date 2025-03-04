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

#include "ExecDomain.h"
#include "Timeout.h"

namespace Nirvana {
namespace Core {

class ExecDomain;
class Synchronized;

/// \brief Implements wait list for synchronized operations with timeout feature.
class EventSyncTimeout : public Timeout <EventSyncTimeout>
{
	static const TimeBase::TimeT TIMER_DEADLINE = 1 * TimeBase::MILLISECOND;

public:
	EventSyncTimeout ();
	~EventSyncTimeout ();

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

public:
	void on_timer (TimeBase::TimeT cur_time) noexcept
	{
		TimeBase::TimeT next_time = NO_EXPIRE;
		for (ListEntry** link = &list_;;) {
			ListEntry* entry = *link;
			if (!entry)
				break;
			if (entry->expire_time >= cur_time) {
				*link = entry->next;
				entry->exec_domain.resume ();
			} else {
				if (next_time > entry->expire_time)
					next_time = entry->expire_time;
				link = &entry->next;
			}
		}
		nearest_expire_time (next_time);
	}

	void on_exception (const CORBA::Exception& exc) noexcept;

private:
	struct ListEntry
	{
		TimeBase::TimeT expire_time;
		ExecDomain& exec_domain;
		ListEntry* next;
		bool result;
	};

	ListEntry* list_;
	size_t signal_cnt_;
};

}
}

#endif
