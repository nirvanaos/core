/*
* Nirvana SQLite module.
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
#ifndef SQLITE_FILESYSTEM_H_
#define SQLITE_FILESYSTEM_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/base64.h>

#define OBJID_PREFIX "id:"
#define VFS_NAME "nirvana"

namespace SQLite {

inline
std::string id_to_string (const Nirvana::DirItemId& id)
{
	return OBJID_PREFIX + base64::encode_into <std::string> (id.begin (), id.end ());
}

}

#endif
