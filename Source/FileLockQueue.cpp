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
#include "FileLockQueue.h"
#include "FileLockRanges.h"

namespace Nirvana {
namespace Core {

void FileLockQueue::retry_lock (FileLockRanges& lock_ranges) noexcept
{
	bool changed = false;
	for (Entry** link = &list_;;) {
		Entry* entry = *link;
		if (!entry)
			break;
		LockType lmin = entry->level_min;
		LockType lmax = entry->level_max;
		try {
			LockType l = lock_ranges.set (entry->begin, entry->end, lmax, lmin, entry->owner);
			if (l >= lmin) {
				*link = entry->next;
				entry->signal (l);
				changed = true;
			} else
				link = &entry->next;
		} catch (const CORBA::Exception& ex) {
			on_exception (ex);
			return;
		}
	}
	if (changed)
		adjust_timer ();
}

void FileLockQueue::adjust_timer () noexcept
{
	if (timer_) {
		TimeBase::TimeT nearest = NO_EXPIRE;
		for (Entry* entry = list_; entry; entry = entry->next) {
			if (nearest > entry->expire_time)
				nearest = entry->expire_time;
		}
		nearest_expire_time (nearest);
	}
}

void FileLockQueue::on_exception (const CORBA::Exception& exc) noexcept
{
	nearest_expire_time (NO_EXPIRE);

	Entry* entry = list_;
	list_ = nullptr;
	while (entry) {
		Entry* next = entry->next;
		entry->exec_domain.resume (exc);
		entry = next;
	}
}

}
}
