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

FileLockRanges::AcqResult FileLockRanges::acquire (const FileLock& fl, const void* owner, AcqOption opt)
{
	LockType level = fl.type ();
	assert (level > LockType::LOCK_NONE);
	FileSize begin = fl.start (), end;
	Ranges::iterator it = get_end (fl, end);
	Ranges::iterator it_next = it;
	Ranges::iterator replace = ranges_.end ();
	bool shared = false;
	while (ranges_.begin () != it) {
		--it;
		if (it->begin >= begin)
			it_next = it;
		if (it->end > begin) {
			LockType cur_level = it->level;
			if (it->owner == owner) {
				if (LockType::LOCK_EXCLUSIVE == level && AcqOption::FROM_QUEUE == opt && cur_level == LockType::LOCK_PENDING)
					replace = it;
				else if (level > LockType::LOCK_SHARED || cur_level > LockType::LOCK_SHARED) {
					assert (AcqOption::FROM_QUEUE != opt);
					return AcqResult::COLLISION;
				}
			} else if (cur_level > LockType::LOCK_RESERVED)
				return AcqResult::DELAY;
			else if (cur_level > LockType::LOCK_SHARED) {
				if (level > LockType::LOCK_SHARED)
					return AcqResult::DELAY;
			} else {
				// Shared lock present
				if (LockType::LOCK_EXCLUSIVE == level) {
					if (AcqOption::USUAL == opt)
						level = LockType::LOCK_PENDING;
					else
						return AcqResult::DELAY;
				}
				shared = true;
			}
		}
	}

	if (replace != ranges_.end ()) {

		// Replace LOCK_PENDING with LOCK_EXCLUSIVE if no shared locks exist
		assert (replace->begin <= begin && end <= replace->end);
		assert (LockType::LOCK_EXCLUSIVE == level);
		assert (LockType::LOCK_PENDING == replace->level);
		
		if (shared)
			return AcqResult::DELAY;

		if (replace->begin == begin) {
			if (end < replace->end)
				ranges_.emplace (lower_bound (end), end, replace->end, owner, LockType::LOCK_PENDING);
			replace->level = LockType::LOCK_EXCLUSIVE;
		} else {
			assert (replace->begin < begin);
			if (end < replace->end) {
				ranges_.reserve (ranges_.size () + 2);
				ranges_.emplace (lower_bound (end), end, replace->end, owner, LockType::LOCK_PENDING);
			}
			ranges_.emplace (lower_bound (begin), begin, end, owner, LockType::LOCK_EXCLUSIVE);
		}

	} else {

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
	}

	return AcqResult::ACQUIRED;
}

bool FileLockRanges::release (const FileLock& fl, const void* owner) noexcept
{
	LockType level = fl.type ();
	assert (level > LockType::LOCK_NONE);
	FileSize begin = fl.start (), end;
	if (0 == fl.len ())
		end = std::numeric_limits <FileSize>::max ();
	else
		end = fl.start () + fl.len ();

	for (Ranges::iterator it = ranges_.begin ();;) {
		if (it == ranges_.end () || it->begin > begin)
			return false;
		if (it->owner == owner && it->level == level && it->begin < end && it->end >= begin) {
			// Found. Releasing range must be a part of the found range.
			if (it->begin > begin || it->end < end)
				return false; // Wrong range

			if (it->begin == begin) {
				if (it->end == end) {
					// Simplest case. Erase and return.
					ranges_.erase (it);
					return true;
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
					ranges_.emplace (lower_bound (end), begin, end, owner, level);
				it->end = begin;
			}

			return true;
		}
	}
}

FileLockRanges::Ranges::iterator FileLockRanges::get_end (const FileLock& fl, FileSize& end) noexcept
{
	Ranges::iterator it;
	if (0 == fl.len ()) {
		end = std::numeric_limits <FileSize>::max ();
		it = ranges_.end ();
	} else {
		end = fl.start () + fl.len ();
		it = lower_bound (end);
	}
	return it;
}

FileLockRanges::Ranges::iterator FileLockRanges::lower_bound (const FileSize& end) noexcept
{
	return std::lower_bound (ranges_.begin (), ranges_.end (), end, Comp ());
}

}
}
