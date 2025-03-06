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

bool FileLockRanges::test_lock (const FileSize begin, const FileSize end,
	LockType level_max, LockType level_min, const void* owner, TestResult& result) const
{
	assert (begin < end);
	assert (level_max > LockType::LOCK_NONE);
	assert (LockType::LOCK_NONE < level_min && level_min <= level_max);

	LockType level = level_max;
	int cur_min = (int)LockType::LOCK_EXCLUSIVE + 1, cur_max = (int)LockType::LOCK_NONE;

	// Scan locks
	FileSize right = end;
	const Ranges::const_iterator it_end = lower_bound (end);
	for (auto it = it_end; ranges_.begin () != it;) {
		--it;
		if (it->end > begin) {
			LockType it_level = it->level;
			if (it->owner != owner) {
				// Other lock
				
				if (it_level >= LockType::LOCK_PENDING)
					return false; // Pending lock prohibits new lock
				else if (it_level == LockType::LOCK_RESERVED) {
					if (level_min > LockType::LOCK_SHARED)
						return false;
					else
						level = LockType::LOCK_SHARED;
				} else {
					// Other shared lock present
					// We can obtain LOCK_PENDING and below
					if (LockType::LOCK_PENDING < level_min)
						return false;
					if (level > LockType::LOCK_PENDING)
						level = LockType::LOCK_PENDING;
				}
			} else {
				// Own lock
				if (cur_max < (int)it_level)
					cur_max = (int)it_level;
				if (it->end < right)
					cur_min = (int)LockType::LOCK_NONE;
				else if (cur_min > (int)it_level)
					cur_min = (int)it_level;
				right = it->begin;
			}
		}
	}

	// The lock is allowed
	result.can_set = level;
	result.cur_max = (LockType)cur_max;
	if (cur_min > (int)LockType::LOCK_EXCLUSIVE)
		result.cur_min = LockType::LOCK_NONE;
	else
		result.cur_min = (LockType)cur_min;

	return true;
}

bool FileLockRanges::unchecked_set (FileSize begin, FileSize end, const void* owner, LockType level)
{
	assert (begin < end);
	bool changed = false;
	bool insert = true;

	if (LockType::LOCK_NONE != level) // We will insert at most 2 elements
		ranges_.reserve (ranges_.size () + 2);

	Ranges::iterator it_begin = ranges_.end ();
	Ranges::iterator it_ub = std::upper_bound (ranges_.begin (), ranges_.end (), end, Comp ());
	for (auto it = it_ub; ranges_.begin () != it;) {
		--it;
		assert (it->begin <= end); // We step left from upper bound
		if (it->owner == owner) {
			if (it->end > end) {
				FileSize tail_end = it->end;
				LockType tail_level = it->level;
				if (tail_level == level) {
					if (it->begin <= begin) {
						insert = false;
						break;
					}
					--it_ub;
					it = ranges_.erase (it);
					end = tail_end;
					changed = true;
				} else if (it->begin < end) {
					--it_ub;
					it = ranges_.erase (it);
					ranges_.emplace (std::upper_bound (it, it_ub, end, Comp ()), end, tail_end, owner, tail_level);
					changed = true;
				}
			} else if (it->end >= begin) {
				if (it->begin > begin) {
					--it_ub;
					it = ranges_.erase (it);
					changed = true;
				} else {
					if (it->begin < begin) {
						if (it->level == level) {
							changed |= it->end != end;
							it->end = end;
							insert = false;
						} else {
							it->end = begin;
							changed = true;
						}
					} else {
						if (level != LockType::LOCK_NONE) {
							changed |= it->level != level || it->end != end;
							it->level = level;
							it->end = end;
						} else {
							--it_ub;
							it = ranges_.erase (it);
							changed = true;
						}
						insert = false;
					}
					break;
				}
			} else
				break;
		}
	}

	if (insert && level != LockType::LOCK_NONE) {
		ranges_.emplace (std::upper_bound (ranges_.begin (), it_ub, begin, Comp ()), begin, end, owner, level);
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
