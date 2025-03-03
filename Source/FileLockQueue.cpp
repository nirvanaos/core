/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include "FileLockQueue.h"
#include "FileLockRanges.h"

namespace Nirvana {
namespace Core {

FileLockQueue::Entry* FileLockQueue::dequeue (Entry* prev, Entry* entry) noexcept
{
	Entry* next = entry->next ();
	if (prev)
		prev->next (next);
	else
		list_ = next;
	return next;
}

void FileLockQueue::retry_lock (FileLockRanges& lock_ranges) noexcept
{
	bool changed = false;
	for (Entry* entry = list_, *prev = nullptr; entry;) {
		LockType lmin = entry->level_min ();
		LockType lmax = entry->level_max ();
		try {
			bool downgraded;
			LockType l = lock_ranges.set (entry->begin (), entry->end (), lmax, lmin, entry->owner (),
				downgraded);
			assert (!downgraded);
			if (l >= lmin) {
				Entry* next = dequeue (prev, entry);
				entry->signal (l);
				entry = next;
				changed = true;
			} else {
				prev = entry;
				entry = entry->next ();
			}
		} catch (const CORBA::Exception& ex) {
			Entry* next = dequeue (prev, entry);
			entry->signal (ex);
			entry = next;
			changed = true;
		}
	}
	if (changed)
		adjust_timer ();
}

void FileLockQueue::adjust_timer () noexcept
{
	if (timer_) {
		TimeBase::TimeT nearest = std::numeric_limits <TimeBase::TimeT>::max ();
		for (Entry* entry = list_; entry; entry = entry->next ()) {
			if (nearest > entry->expire_time ())
				nearest = entry->expire_time ();
		}
		nearest_expire_time (nearest);
	}
}

void FileLockQueue::nearest_expire_time (TimeBase::TimeT nearest) noexcept
{
	if (nearest_expire_time_ != nearest) {
		if (!list_) {
			nearest_expire_time_ = std::numeric_limits <TimeBase::TimeT>::max ();
			timer_->cancel ();
		} else {
			nearest_expire_time_ = nearest;
			TimeBase::TimeT timeout = std::min (nearest - Chrono::steady_clock (), CHECK_TIMEOUT);
			try {
				timer_->set (0, timeout, 0);
			} catch (const CORBA::Exception& exc) {
				signal_all (exc);
			}
		}
	}
}

void FileLockQueue::signal_all (const CORBA::Exception& exc) noexcept
{
	while (list_) {
		Entry* entry = list_;
		list_ = entry->next ();
		entry->signal (exc);
	}
}

void FileLockQueue::Timer::run (const TimeBase::TimeT&)
{
	if (obj_)
		obj_->on_timer ();
}

}
}
