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
#include "pch.h"
#include "filesystem.h"
#include "Global.h"
#include "sqlite/sqlite3.h"
#include <fnctl.h>

extern "C" int sqlite3_os_init ()
{
	return SQLITE_OK;
}

extern "C" int sqlite3_os_end ()
{
	return SQLITE_OK;
}

namespace SQLite {

const char* is_id (const char* file) noexcept
{
	const char* prefix_end = file + std::size (OBJID_PREFIX);
	if (std::equal (file, prefix_end, OBJID_PREFIX))
		return prefix_end;
	return nullptr;
}

inline
Nirvana::DirItemId string_to_id (const char* begin, const char* end)
{
	return base64::decode_into <Nirvana::DirItemId> (begin, end);
}

struct File : sqlite3_file
{
	int close () noexcept
	{
		int err = EIO;
		if (access) {
			try {
				access->flush ();
				access = nullptr;
				return SQLITE_OK;
			} catch (const CORBA::SystemException& ex) {
				int e = Nirvana::get_minor_errno (ex.minor ());
				if (e)
					err = e;
			} catch (...) {
			}
		}
		return err;
	}

	Nirvana::AccessDirect::_ref_type access;
};

int close (sqlite3_file* p) noexcept
{
	if (p)
		static_cast <File&> (*p).close ();
	return SQLITE_OK;
}

const sqlite3_io_methods io_methods = {
	3,
	&close
};

int xOpen (sqlite3_vfs*, sqlite3_filename zName, sqlite3_file* file,
	int flags, int* pOutFlags) noexcept
{
	Nirvana::Access::_ref_type fa;
	try {
		const char* id_begin = is_id (zName);
		if (id_begin) {
			Nirvana::File::_ref_type file = Nirvana::File::_narrow (
				global.file_system ()->get_item (
					string_to_id (id_begin, id_begin + strlen (id_begin))));
			if (!file)
				return SQLITE_CANTOPEN;
			fa = file->open (O_RDWR | O_DIRECT, 0);
		} else {
			uint_fast16_t open_flags = O_RDWR | O_DIRECT;
			if (flags & SQLITE_OPEN_CREATE)
				open_flags |= O_CREAT;
			if (flags & SQLITE_OPEN_EXCLUSIVE)
				open_flags |= O_EXCL;
			fa = global.open_file (zName, open_flags);
		}
	} catch (CORBA::NO_MEMORY ()) {
		return SQLITE_NOMEM;
	} catch (...) {
		return SQLITE_CANTOPEN;
	}
	File& f = static_cast <File&> (*file);
	f.pMethods = &io_methods;
	f.access = Nirvana::AccessDirect::_narrow (fa->_to_object ());
	return SQLITE_OK;
}

struct sqlite3_vfs vfs = {
	3,                   /* Structure version number (currently 3) */
	(int)sizeof (File),  /* Size of subclassed sqlite3_file */
  1024,                /* Maximum file pathname length */
	nullptr,             /* Next registered VFS */
	VFS_NAME,            /* Name of this virtual file system */
	nullptr,             /* Pointer to application-specific data */
};

}
