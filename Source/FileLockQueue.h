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
#ifndef NIRVANA_CORE_FILELOCKQUEUE_H_
#define NIRVANA_CORE_FILELOCKQUEUE_H_
#pragma once

#include "ExecDomain.h"
#include <Nirvana/File.h>
#include "Timeout.h"

namespace Nirvana {
namespace Core {

class FileLockRanges;

class FileLockQueue : public Timeout <FileLockQueue>
{
public:
	FileLockQueue () :
		list_ (nullptr)
	{}

	~FileLockQueue ()
	{
		on_exception (CORBA::OBJECT_NOT_EXIST ());
	}

	struct Entry
	{
	public:
		void signal (LockType level) noexcept
		{
			level_max = level;
			exec_domain.resume ();
		}

		void signal (const CORBA::Exception& exc) noexcept
		{
			exec_domain.resume (exc);
		}

		FileSize begin;
		FileSize end;
		LockType level_max;
		LockType level_min;
		TimeBase::TimeT expire_time;
		ExecDomain& exec_domain;
		const void* owner;
		Entry* next;
	};

	LockType enqueue (const FileSize& begin, const FileSize& end,
		LockType level_max, LockType level_min,
		const void* owner, const TimeBase::TimeT& timeout)
	{
		Entry entry { begin, end, level_max, level_min,
			std::numeric_limits <TimeBase::TimeT>::max (), ExecDomain::current (), owner, nullptr };

		if (timeout != std::numeric_limits <TimeBase::TimeT>::max ()) {
			TimeBase::TimeT cur_time = Chrono::steady_clock ();
			if (std::numeric_limits <TimeBase::TimeT>::max () - cur_time <= timeout)
				throw_BAD_PARAM ();
			entry.expire_time = cur_time + timeout;
		}

		DeadlineTime deadline = entry.exec_domain.deadline ();

		Entry** link = &list_;
		for (;;) {
			Entry* next = *link;
			if (!next || next->exec_domain.deadline () > deadline)
				break;
			link = &(next->next);
		}

		if (entry.expire_time < nearest_expire_time ())
			nearest_expire_time (entry.expire_time);

		entry.exec_domain.suspend_prepare ();

		entry.next = *link;
		*link = &entry;

		entry.exec_domain.suspend ();

		return entry.level_max;
	}
	
	bool empty () const noexcept
	{
		return !list_;
	}

	void on_delete_proxy (const void* owner) noexcept
	{
		bool changed = false;
		for (Entry** link = &list_;;) {
			Entry* entry = *link;
			if (!entry)
				break;
			if (entry->owner == owner) {
				*link = entry->next;
				entry->signal (LockType::LOCK_NONE);
				changed = true;
			} else
				link = &entry->next;
		}
		if (changed)
			adjust_timer ();
	}

	void retry_lock (FileLockRanges& lock_ranges) noexcept;

	void on_timer (TimeBase::TimeT cur_time) noexcept
	{
		TimeBase::TimeT next_time = NO_EXPIRE;
		for (Entry** link = &list_;;) {
			Entry* entry = *link;
			if (!entry)
				break;
			if (entry->expire_time >= cur_time) {
				*link = entry->next;
				entry->signal (LockType::LOCK_NONE);
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
	void adjust_timer () noexcept;

private:
	Entry* list_;
};

}
}

#endif
