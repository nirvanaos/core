/*
* Nirvana SQLite driver.
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
#ifndef SQLITE_WRITABLESTATICDATA_H_
#define SQLITE_WRITABLESTATICDATA_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include "../Source/MapUnorderedUnstable.h"
#include <CORBA/TimeBase.h>

namespace SQLite {

class WritableStaticData
{
public:
	void* get (const void* key, size_t size)
	{
		auto ins = map_.emplace (key, 0);
		if (ins.second) {
			try {
				size_t offset = data_.size ();
				if (size >= 8)
					offset = Nirvana::round_up (offset, (size_t)8);
				else if (size >= 4)
					offset = Nirvana::round_up (offset, (size_t)4);
				else if (size >= 2)
					offset = Nirvana::round_up (offset, (size_t)2);
				data_.resize (offset + size);
				memcpy (data_.data () + offset, key, size);
				ins.first->second = offset;
			} catch (...) {
				map_.erase (ins.first);
				throw;
			}
		}
		return data_.data () + ins.first->second;
	}
	
private:
	typedef Nirvana::Core::MapUnorderedUnstable <const void*, size_t> Map;
	Map map_;
	std::vector <uint8_t> data_;
};

}

#endif
