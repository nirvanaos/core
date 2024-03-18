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
#include "FileLockMap.h"

namespace Nirvana {
namespace Core {

bool FileLockMap::acquire (const FileLock& fl, const void* owner)
{
	LockType level = fl.type ();
	assert (level > LockType::LOCK_NONE);
	FileSize begin = fl.start (), end;
	Ranges::iterator it = get_end (fl, end);
	Ranges::iterator hint = it;
	while (ranges_.begin () != it) {
		--it;
		if (it->begin >= begin)
			hint = it;
		if (it->end > begin) {
			if (LockType::LOCK_EXCLUSIVE == level)
				return false;
			else {
				LockType cur_level = it->level;
				if (level > LockType::LOCK_SHARED) {
					if (cur_level > LockType::LOCK_SHARED)
						return false;
				} else if (cur_level > LockType::LOCK_RESERVED)
					return false;
			}
		}
	}

	// We cah acquire lock
	ranges_.emplace (hint, begin, end, level, owner);
	return true;
}
/*
bool FileLockMap::release (const FileLock& fl, const void* owner) noexcept
{
	LockType level = fl.type ();
	assert (level > LockType::LOCK_NONE);
	FileSize begin = fl.start (), end;
	Ranges::iterator it_end = get_end (fl, end);
	Ranges::iterator it = it_end;
	while (ranges_.begin () != it) {
		--it;
		if (it->end > begin) {
			if (it->level == level && it->owner == owner && it->end >= end) {

			}
		}
	}
}
*/
FileLockMap::Ranges::iterator FileLockMap::get_end (const FileLock& fl, FileSize& end) noexcept
{
	Ranges::iterator it;
	if (0 == fl.len ()) {
		end = std::numeric_limits <FileSize>::max ();
		it = ranges_.end ();
	} else {
		end = fl.start () + fl.len ();
		it = std::lower_bound (ranges_.begin (), ranges_.end (), end, Comp ());
	}
	return it;
}

FileLockMap::Ranges::const_iterator FileLockMap::get_end (const FileSize& end) const noexcept
{
	return std::lower_bound (ranges_.begin (), ranges_.end (), end, Comp ());
}

}
}
