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
#include <algorithm>

namespace Nirvana {
namespace Core {

LockType FileLockRanges::set (const FileSize& begin, const FileSize& end,
	LockType level_max, LockType level_min, const void* owner, bool& downgraded)
{
	assert (begin < end);
	assert (level_max > LockType::LOCK_NONE);
	assert (LockType::LOCK_NONE < level_min && level_min <= level_max);

	const Ranges::iterator it_end = lower_bound (end);

	LockType level = level_max;

	// Scan other locks
	LockType cur_min_level = LockType::LOCK_NONE, cur_max_level = LockType::LOCK_NONE;
	for (auto it = it_end; ranges_.begin () != it;) {
		--it;
		if (it->end > begin) {
			if (it->owner != owner) {
				LockType other_level = it->level;
				if (other_level >= LockType::LOCK_PENDING)
					return LockType::LOCK_NONE;
				else if (other_level > LockType::LOCK_SHARED) {
					if (level_min > LockType::LOCK_SHARED)
						return LockType::LOCK_NONE;
					else
						level = LockType::LOCK_SHARED;
				} else {
					// Other shared lock present
					if (LockType::LOCK_EXCLUSIVE == level) {
						if (LockType::LOCK_EXCLUSIVE == level_min)
							return LockType::LOCK_NONE;
						else
							level = LockType::LOCK_PENDING;
					}
				}
			} else {
				LockType own_level = it->level;
				if (LockType::LOCK_NONE == cur_min_level) {
					cur_min_level = own_level;
					cur_max_level = own_level;
				} else {
					if (cur_min_level > own_level)
						cur_min_level = own_level;
					if (cur_max_level < own_level)
						cur_max_level = own_level;
				}
			}
		}
	}

	// We can set the lock

	if (cur_min_level >= level && cur_max_level <= level_max)
		return cur_min_level; // Nothing to change

	unchecked_set (begin, end, it_end, owner, level);

	downgraded = level < cur_max_level;

	return level;
}

bool FileLockRanges::unchecked_set (const FileSize& begin, const FileSize& end,
	Ranges::iterator it_end, const void* owner, LockType level)
{
	assert (begin < end);

	if (LockType::LOCK_NONE != level) // Insert at most 2 elements
		ranges_.reserve (ranges_.size () + 2);

	Ranges::iterator it_last = ranges_.end (), it_first = ranges_.end ();
	bool changed = false;
	for (auto it = it_end; ranges_.begin () != it;) {
		--it;
		if (it->owner == owner && it->end > begin) {
			if (it->begin <= begin)
				it_first = it;
			if (it->end >= end)
				it_last = it;
			if (it != it_first && it != it_last) {
				it = ranges_.erase (it);
				changed = true;
			}
		}
	}
	if (it_first != ranges_.end ()) {
		if (it_last == it_first) {
			if (it_first->level != level) {
				if (it_first->begin == begin && it_first->end == end) {
					if (LockType::LOCK_NONE != level)
						it_first->level = level;
					else
						ranges_.erase (it_first);
				}
				it_last = ranges_.emplace (std::upper_bound (it_first + 1, ranges_.end (), end, Comp ()), end, it_last->end, owner, it_last->level);
				if (LockType::LOCK_NONE != level)
					ranges_.emplace (std::upper_bound (it_first + 1, it_last, begin, Comp ()), begin, end, owner, level);
				changed = true;
			}
			it_last = ranges_.end ();
		} else
			it_first->end = begin;
	}
	if (it_last != ranges_.end ()) {
		assert (it_last->begin > begin);
		if (LockType::LOCK_NONE != level && (it_last->level == level || it_last->end == end)) {
			// Extend begin
			it_last->begin = begin;
			// Bubble
			while (it_last != ranges_.begin ()) {
				auto prev = it_last - 1;
				if (prev->begin > begin) {
					std::swap (*prev, *it_last);
					it_last = prev;
				} else
					break;
			}
		} else {
			if (it_last->end > end) {
				it_last->begin = end;
				// Bubble
				for (;;) {
					auto next = it_last + 1;
					if (next != ranges_.end () && next->begin < end) {
						std::swap (*next, *it_last);
						it_last = next;
					} else
						break;
				}
				if (LockType::LOCK_NONE != level)
					ranges_.emplace (std::upper_bound (ranges_.begin (), it_last, begin, Comp ()), begin, end, owner, level);
			} else {
				assert (LockType::LOCK_NONE == level);
				ranges_.erase (it_last);
			}
		}
		changed = true;
	}
	return changed;
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
