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
#include "MapOrderedUnstable.h"
#include "UserAllocator.h"

namespace Nirvana {
namespace Core {

class FileLockMap
{
public:
	bool acquire (const FileLock& fl, const void* owner)
	{
		LockType type = fl.type ();
		assert (type > LockType::LOCK_NONE);
		FileSize begin = fl.start (), end;
		Map::iterator it_end = get_end (fl, end);
		Map::iterator it_begin = it_end;
		while (map_.begin () != it_begin) {
			--it_begin;
			if (it_begin->second.end < begin) {
				if (type > LockType::LOCK_SHARED)
					return false;
			} else {
				++it_begin;
				break;
			}
		}
	}

	void release (const FileLock& fl)
	{
	}

private:
	struct Entry {
		FileSize end;
		unsigned shared_cnt;
		LockType level;
		const void* owner;
	};

	typedef MapOrderedUnstable <FileSize, Entry, std::less <FileSize>, UserAllocator> Map;

	Map::iterator get_end (const FileLock& fl, FileSize& end) noexcept
	{
		Map::iterator it;
		if (0 == fl.len ()) {
			end = std::numeric_limits <FileSize>::max ();
			it = map_.end ();
		} else {
			end = fl.start () + fl.len ();
			it = map_.lower_bound (end);
		}
		return it;
	}

private:
	Map map_;
};

}
}

#endif
