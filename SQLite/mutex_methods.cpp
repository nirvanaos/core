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
#include "mutex_methods.h"
#include <assert.h>
#include <Nirvana/System.h>

// For debugging

struct sqlite3_mutex
{
	size_t exec_domain_id;
	size_t cnt;
	int type;
	bool dynamic;
};

using namespace Nirvana;

namespace SQLite {

const size_t STATIC_MUTEX_CNT = SQLITE_MUTEX_STATIC_VFS3 - SQLITE_MUTEX_RECURSIVE;

sqlite3_mutex static_mutexes [STATIC_MUTEX_CNT];

extern "C" int xMutexInit (void)
{
	return SQLITE_OK;
}

extern "C" int xMutexEnd (void)
{
	return SQLITE_OK;
}

extern "C" sqlite3_mutex* xMutexAlloc (int type)
{
	int static_mutex = type - SQLITE_MUTEX_RECURSIVE - 1;
	if (static_mutex >= 0)
		return static_mutexes + static_mutex;
	else {
		sqlite3_mutex* pm = (sqlite3_mutex*)sqlite3_malloc (sizeof (sqlite3_mutex));
		pm->exec_domain_id = 0;
		pm->cnt = 0;
		pm->type = type;
		pm->dynamic = true;
		return pm;
	}
}

extern "C" void xMutexFree (sqlite3_mutex * pm)
{
	if (pm->dynamic)
		sqlite3_free (pm);
}

extern "C" void xMutexEnter (sqlite3_mutex * pm)
{
	if (pm->dynamic) {
		size_t ed_id = Nirvana::g_system->exec_domain_id ();
		assert (!pm->cnt || pm->exec_domain_id == ed_id);
		if (!pm->cnt++)
			pm->exec_domain_id = ed_id;
	}
}

extern "C" int xMutexTry (sqlite3_mutex * pm)
{
	sqlite3_mutex_enter (pm);
	return 0;
}

extern "C" void xMutexLeave (sqlite3_mutex * pm)
{
	if (pm->dynamic) {
		assert (pm->cnt && pm->exec_domain_id == Nirvana::g_system->exec_domain_id ());
		--(pm->cnt);
	}
}

extern "C" int xMutexHeld (sqlite3_mutex * pm)
{
	return !pm->dynamic ||
		(pm->cnt && pm->exec_domain_id == Nirvana::g_system->exec_domain_id ());
}

extern "C" int xMutexNotheld (sqlite3_mutex * pm)
{
	return 0 == xMutexHeld (pm);
}

const struct sqlite3_mutex_methods mutex_methods = {
	xMutexInit,
	xMutexEnd,
	xMutexAlloc,
	xMutexFree,
	xMutexEnter,
	xMutexTry,
	xMutexLeave,
	xMutexHeld,
	xMutexNotheld
};

}
