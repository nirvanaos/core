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
#include "Global.h"
#include "mem_methods.h"
#include "mutex_methods.h"
#include "filesystem.h"
#include <Nirvana/System.h>
#include <Nirvana/POSIX.h>
#include <signal.h>

namespace SQLite {

inline
Global::Global () :
	file_system_ (Nirvana::FileSystem::_narrow (CosNaming::NamingContext::_narrow (
		CORBA::the_orb->resolve_initial_references ("NameService"))->resolve (CosNaming::Name (1)))),
	cs_key_ (-1)
{
	sqlite3_config (SQLITE_CONFIG_PAGECACHE, nullptr, 0, 0);
	sqlite3_config (SQLITE_CONFIG_MALLOC, &mem_methods);
	sqlite3_config (SQLITE_CONFIG_MUTEX, &mutex_methods);
	sqlite3_vfs_register (&vfs, 1);
	sqlite3_initialize ();

	cs_key_ = Nirvana::the_system->CS_alloc (wsd_deleter);
}

inline
Global::~Global ()
{
	sqlite3_shutdown ();
	Nirvana::the_system->CS_free (cs_key_);
}

Global global;

Nirvana::Access::_ref_type Global::open_file (const char* url, uint_fast16_t flags) const
{
	// Get full path name
	CosNaming::Name name;
	Nirvana::the_posix->append_path (name, url, true);

	// Open file
	name.erase (name.begin ());
	return file_system_->open (name, flags, 0);
}

void Global::wsd_deleter (void* p)
{
	delete reinterpret_cast <WritableStaticData*> (p);
}

WritableStaticData& Global::static_data ()
{
	if (cs_key_ < 0) {
		// Initialization stage
		return initial_static_data_;
	} else {
		void* p = Nirvana::the_system->CS_get (cs_key_);
		if (!p) {
			p = new WritableStaticData (initial_static_data_);
			Nirvana::the_system->CS_set (cs_key_, p);
		}
		return *reinterpret_cast <WritableStaticData*> (p);
	}
}

}

extern "C" int sqlite3_wsd_init (int N, int J)
{
	return SQLITE_OK;
}

extern "C" void* sqlite3_wsd_find (void* K, int L)
{
	try {
		return SQLite::global.static_data ().get (K, L);
	} catch (...) {
		Nirvana::the_posix->raise (SIGABRT);
	}
	return nullptr;
}

