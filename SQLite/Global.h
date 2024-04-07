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
#ifndef SQLITE_GLOBAL_H_
#define SQLITE_GLOBAL_H_
#pragma once

#include "WritableStaticData.h"
#include <Nirvana/File.h>

namespace SQLite {

/// Module global data
/// 
class Global
{
public:
	Global ();
	~Global ();

	Nirvana::FileSystem::_ptr_type file_system () const noexcept
	{
		assert (file_system_);
		return file_system_;
	}

	Nirvana::AccessDirect::_ref_type open_file (const char* path, uint_fast16_t flags) const;

	WritableStaticData& static_data ();

private:
	static void wsd_deleter (void*);

private:
	Nirvana::FileSystem::_ref_type file_system_;
	WritableStaticData initial_static_data_;
	int cs_key_;
};

extern Global global;

}

#endif
