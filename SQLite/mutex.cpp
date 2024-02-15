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
#include <assert.h>
#include <Nirvana/System.h>
#include "sqlite/sqlite3.h"

// For debugging

struct sqlite3_mutex
{
	size_t exec_domain_id;
	size_t cnt;
};

extern "C" sqlite3_mutex* sqlite3_mutex_alloc (int)
{
	sqlite3_mutex* pm = (sqlite3_mutex*)sqlite3_malloc (sizeof (sqlite3_mutex));
	pm->exec_domain_id = 0;
	pm->cnt = 0;
	return pm;
}

extern "C" void sqlite3_mutex_free (sqlite3_mutex* pm)
{
	sqlite3_free (pm);
}

extern "C" void sqlite3_mutex_enter (sqlite3_mutex* pm)
{
	size_t ed_id = Nirvana::g_system->exec_domain_id ();
	assert (!pm->cnt || pm->exec_domain_id == ed_id);
	if (pm->cnt++)
		pm->exec_domain_id = ed_id;
}

extern "C" int sqlite3_mutex_try (sqlite3_mutex* pm)
{
	sqlite3_mutex_enter (pm);
	return 0;
}

extern "C" void sqlite3_mutex_leave (sqlite3_mutex* pm)
{
	size_t ed_id = Nirvana::g_system->exec_domain_id ();
	assert (pm->cnt && pm->exec_domain_id == ed_id);
	--(pm->cnt);
}
