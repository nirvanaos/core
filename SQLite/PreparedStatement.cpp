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
#include "PreparedStatement.h"

namespace SQLite {

void PreparedStatement::check_exist () const
{
	if (!stmt_)
		throw CORBA::OBJECT_NOT_EXIST ();
}

unsigned PreparedStatement::get_param_index (const IDL::String& name) const
{
	check_exist ();
	int idx = sqlite3_bind_parameter_index (stmt_, name.c_str ());
	if (idx < 1)
		throw NDBC::SQLException (NDBC::SQLWarning (0, "Parameter not found: " + name), NDBC::SQLWarnings ());
	return idx;
}

}
