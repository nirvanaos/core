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
#ifndef NIRVANA_CORE_FILELOCKRANGES_H_
#define NIRVANA_CORE_FILELOCKRANGES_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/File.h>
#include <vector>

namespace Nirvana {
namespace Core {

class FileLockRanges
{
public:
	struct TestResult
	{
		LockType can_set, cur_max, cur_min;
	};

	bool test_lock (const FileSize begin, const FileSize end,
		LockType level_max, LockType level_min, const void* owner,
		TestResult& result) const noexcept;

	LockType set (const FileSize& begin, const FileSize& end,
		LockType level_max, LockType level_min, const void* owner)
	{
		LockType ret = LockType::LOCK_NONE;
		TestResult test_result;
		if (test_lock (begin, end, level_max, level_min, owner, test_result)) {
			ret = test_result.can_set;
			if (test_result.cur_max != ret || test_result.cur_min != ret)
				unchecked_set (begin, end, owner, ret);
		}
		return ret;
	}

	bool clear (const FileSize& begin, const FileSize& end, const void* owner)
	{
		return unchecked_set (begin, end, owner, LockType::LOCK_NONE);
	}

	bool unchecked_set (FileSize begin, FileSize end, const void* owner, LockType level);

	bool get (const FileSize& begin, const FileSize& end, const void* owner, LockType level, FileLock& out)
		const noexcept
	{
		assert (begin < end);
		assert (level > LockType::LOCK_NONE);
		Ranges::const_iterator found = ranges_.end ();
		for (auto it = lower_bound (end); ranges_.begin () != it; ) {
			--it;
			if (it->end > begin && it->owner != owner) {
				LockType other_level = it->level;
				if (other_level >= LockType::LOCK_PENDING) {
					found = it;
					break;
				} else if (other_level > LockType::LOCK_SHARED) {
					if (level > LockType::LOCK_SHARED) {
						found = it;
						break;
					}
				} else if (level == LockType::LOCK_EXCLUSIVE) {
					found = it;
					break;
				}
			}
		}

		if (found != ranges_.end ()) {
			out.start (found->begin);
			out.len (found->end == std::numeric_limits <FileSize>::max () ? 0 : found->end - found->begin);
			out.type (found->level);
			return true;
		}
		return false;
	}

	bool check_read (const FileSize& begin, const FileSize& end, const void* owner) const noexcept
	{
		Ranges::const_iterator it = lower_bound (end);
		while (it != ranges_.begin ()) {
			--it;
			if (it->end > begin && it->level == LockType::LOCK_EXCLUSIVE && it->owner != owner)
				return false;
		}
		return true;
	}

	bool check_write (const FileSize& begin, const FileSize& end, const void* owner) const noexcept
	{
		Ranges::const_iterator it = lower_bound (end);
		while (it != ranges_.begin ()) {
			--it;
			if (it->end > begin && (it->level < LockType::LOCK_EXCLUSIVE || it->owner != owner))
				return false;
		}
		return true;
	}

	bool delete_all (const void* owner) noexcept
	{
		bool changed = false;
		for (auto it = ranges_.rbegin (); it != ranges_.rend (); ++it) {
			if (it->owner == owner) {
				changed = true;
				ranges_.erase (it.base ());
			}
		}
		return changed;
	}

	std::vector <FileLock> get_all (const void* owner) const
	{
		std::vector <FileLock> ret;
		for (const Entry& e : ranges_) {
			if (e.owner == owner) {
				ret.emplace_back (e.begin, e.end == std::numeric_limits <FileSize>::max ()
					? 0 : e.end - e.begin, e.level);
			}
		}
		return ret;
	}

	enum class SharedLocks {
		NO_LOCKS,
		INSIDE,
		OUTSIDE
	};

	SharedLocks has_shared_locks (const FileSize& begin, const FileSize& end, const void* owner) const noexcept
	{
		SharedLocks ret = SharedLocks::NO_LOCKS;
		for (const Entry& e : ranges_) {
			if (e.owner == owner && (e.level == LockType::LOCK_SHARED || e.level == LockType::LOCK_RESERVED)) {
				if (e.begin < begin || e.end > end) {
					ret = SharedLocks::OUTSIDE;
					break;
				} else
					ret = SharedLocks::INSIDE;
			}
		}
		return ret;
	}

private:
	struct Entry {
		FileSize begin;
		FileSize end;
		const void* owner;
		LockType level;

		Entry (const FileSize& _begin, const FileSize& _end, const void* _owner, LockType _level)
			noexcept :
			begin (_begin),
			end (_end),
			owner (_owner),
			level (_level)
		{}

		bool operator < (const Entry& rhs) const noexcept
		{
			return begin < rhs.begin;
		}
	};

	struct Comp
	{
		bool operator () (const Entry& l, const Entry& r) const noexcept
		{
			return l.begin < r.begin;
		}

		bool operator () (const Entry& l, const FileSize& r) const noexcept
		{
			return l.begin < r;
		}

		bool operator () (const FileSize& l, const Entry& r) const noexcept
		{
			return l < r.begin;
		}
	};

	typedef std::vector <Entry> Ranges;

	Ranges::iterator lower_bound (FileSize end) noexcept;

	Ranges::const_iterator lower_bound (const FileSize& end) const noexcept
	{
		return const_cast <FileLockRanges&> (*this).lower_bound (end);
	}

private:
	Ranges ranges_;
};

}
}

#endif
