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
#include "FileLockRanges.h"

namespace Nirvana {
namespace Core {

LockType FileLockRanges::acquire (const FileSize& begin, const FileSize& end,
	LockType level_max, LockType level_min, const void* owner)
{
	assert (level_max > LockType::LOCK_NONE);
	assert (LockType::LOCK_NONE < level_min && level_min <= level_max);

	Ranges::iterator it = lower_bound (end);
	Ranges::iterator it_next = it; // Where to insert
	LockType level = level_max;
	while (ranges_.begin () != it) {
		--it;
		if (it->begin > begin)
			it_next = it;
		if (it->end > begin) {
			LockType cur_level = it->level;
			if (cur_level >= LockType::LOCK_PENDING)
				return LockType::LOCK_NONE;
			else if (cur_level > LockType::LOCK_SHARED) {
				if (level_min > LockType::LOCK_SHARED)
					return LockType::LOCK_NONE;
				else
					level = LockType::LOCK_SHARED;
			} else {
				// Shared lock present

				// Owner of shared lock may not acquire other lock from scratch.
				// It must upgrade existent shared lock.
				if (it->owner == owner && level_max > LockType::LOCK_SHARED)
					throw_BAD_INV_ORDER (make_minor_errno (EACCES));

				if (LockType::LOCK_EXCLUSIVE == level) {
					if (LockType::LOCK_EXCLUSIVE == level_min)
						return LockType::LOCK_NONE;
					else
						level = LockType::LOCK_PENDING;
				}
			}
		}
	}

	// We can acquire lock

	Ranges::iterator merge_prev = ranges_.end ();
	for (it = it_next; it != ranges_.begin ();) {
		--it;
		if (it->owner == owner && it->level == level && it->end == begin) {
			merge_prev = it;
			break;
		}
	}

	Ranges::iterator merge_next = ranges_.end ();
	for (it = it_next; it != ranges_.end () && it->begin <= end; ++it) {
		if (it->owner == owner && it->level == level && it->begin == end) {
			merge_next = it;
			break;
		}
	}

	if (merge_prev != ranges_.end ()) {
		// Merge to prev
		if (merge_next != ranges_.end ()) {
			merge_prev->end = merge_next->end;
			ranges_.erase (merge_next);
		} else
			merge_prev->end = end;
	} else if (merge_next != ranges_.end ()) {

		// Merge to next
		merge_next->begin = begin;

		// Bubble re-sort
		while (merge_next != ranges_.begin ()) {
			Ranges::iterator prev = merge_next - 1;
			if (begin < prev->begin) {
				std::swap (*merge_next, *prev);
				merge_next = prev;
			} else
				break;
		}

	} else {
		// Just insert a new entry
		ranges_.emplace (it_next, begin, end, owner, level);
	}

	return level;
}

LockType FileLockRanges::replace (const FileSize& begin, const FileSize& end,
	LockType level_old, LockType level_new, const void* owner)
{
	assert (level_old > LockType::LOCK_NONE);

	Ranges::iterator it = lower_bound (end);
	Ranges::iterator found = ranges_.end ();
	while (ranges_.begin () != it) {
		--it;
		if (it->owner == owner && it->level == level_old && it->begin < end && it->end >= begin) {
			found = it;
			if (level_new <= level_old)
				break;
		} else if (level_new > level_old && it->end > begin) {
			LockType cur_level = it->level;
			if (cur_level > LockType::LOCK_SHARED) {
				assert (level_old == LockType::LOCK_SHARED);
				assert (level_new > LockType::LOCK_SHARED);
				return level_old;
			} else {
				// Shared lock present
				if (LockType::LOCK_EXCLUSIVE == level_new) {
					level_new = LockType::LOCK_PENDING;
					if (level_old == level_new)
						return level_old;
				}
			}
		}
	}

	if (found == ranges_.end ())
		throw_BAD_PARAM (make_minor_errno (ENOLCK));

	assert (found->begin <= begin && end <= found->end);
	
	if (level_old == level_new) // Dry check
		return level_old;

	if (level_new == LockType::LOCK_NONE) {
		// Remove lock
		if (found->begin == begin) {
			if (found->end == end) {
				// Simplest case. Erase and return.
				ranges_.erase (it);
			} else {
				assert (it->end > end);
				it->begin = end;
				// Bubble re-sort
				while (it != ranges_.end ()) {
					Ranges::iterator next = it + 1;
					if (end < next->begin) {
						std::swap (*it, *next);
						it = next;
					} else
						break;
				}
			}
		} else {
			assert (it->begin < begin);
			if (it->end > end)  // Need to insert a new entry
				ranges_.emplace (lower_bound (end), begin, end, owner, level_old);
			it->end = begin;
		}
	} else {
		if (found->begin == begin) {
			if (end < found->end)
				ranges_.emplace (lower_bound (end), end, found->end, owner, level_old);
			found->level = level_new;
		} else {
			assert (found->begin < begin);
			if (end < found->end) {
				ranges_.reserve (ranges_.size () + 2);
				ranges_.emplace (lower_bound (end), end, found->end, owner, level_old);
			}
			ranges_.emplace (lower_bound (begin), begin, end, owner, level_new);
		}
	}

	return level_new;
}

FileLockRanges::Ranges::iterator FileLockRanges::lower_bound (FileSize end) noexcept
{
	if (end == std::numeric_limits <FileSize>::max ())
		return ranges_.end ();
	else
		return std::lower_bound (ranges_.begin (), ranges_.end (), end, Comp ());
}

}
}
