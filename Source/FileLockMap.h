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
#ifndef NIRVANA_CORE_FILELOCKMAP_H_
#define NIRVANA_CORE_FILELOCKMAP_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/File.h>
#include <vector>
#include <algorithm>
#include "UserAllocator.h"

namespace Nirvana {
namespace Core {

class FileLockMap
{
public:
	bool acquire (const FileLock& fl, const void* owner);
	bool release (const FileLock& fl, const void* owner) noexcept;

	bool check_read (const FileSize& begin, const FileSize& end, const void* proxy) const noexcept
	{
		Ranges::const_iterator it = get_end (end);
		while (it != ranges_.begin ()) {
			--it;
			if (it->end > begin &&
				it->level == LockType::LOCK_EXCLUSIVE && it->owner != proxy
				)
				return false;
		}
		return true;
	}

	bool check_write (const FileSize& begin, const FileSize& end, const void* proxy) const noexcept
	{
		Ranges::const_iterator it = get_end (end);
		while (it != ranges_.begin ()) {
			--it;
			if (it->end > begin &&
				it->level < LockType::LOCK_EXCLUSIVE || it->owner != proxy
				)
				return false;
		}
		return true;
	}

private:
	struct Entry {
		FileSize begin;
		FileSize end;
		const void* owner;
		LockType level;

		Entry (const FileSize& _begin, const FileSize& _end, LockType _level, const void* _owner)
			noexcept :
			begin (_begin),
			end (_end),
			owner (_owner),
			level (_level)
		{}
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

	typedef std::vector <Entry, UserAllocator <Entry> > Ranges;

	Ranges::iterator get_end (const FileLock& fl, FileSize& end) noexcept;
	Ranges::const_iterator get_end (const FileSize& end) const noexcept;

private:
	Ranges ranges_;
};

}
}

#endif
