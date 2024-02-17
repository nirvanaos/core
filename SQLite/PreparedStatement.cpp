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

PreparedStatement::ParamIndex PreparedStatement::get_param_index (unsigned i) const
{
	check_exist ();
	for (const auto& st : statements ()) {
		auto stmt = st.stmt;
		int par_cnt = sqlite3_bind_parameter_count (stmt);
		if (i <= (unsigned)par_cnt)
			return { stmt, i };
		i -= par_cnt;
	}
	throw NDBC::SQLException (NDBC::SQLWarning (0, "Invalid parameter index"), NDBC::SQLWarnings ());
}

PreparedStatement::ParamIndex PreparedStatement::get_param_index (const IDL::String& name) const
{
	check_exist ();
	for (auto st : statements ()) {
		auto stmt = st.stmt;
		int i = sqlite3_bind_parameter_index (stmt, name.c_str ());
		if (i >= 1)
			return { stmt, (unsigned)i };
	}
	throw NDBC::SQLException (NDBC::SQLWarning (0, "Parameter not found: " + name), NDBC::SQLWarnings ());
}

void PreparedStatement::clearParameters ()
{
	check_exist ();
	for (auto st : statements ()) {
		sqlite3_clear_bindings (st.stmt);
		CORBA::servant_reference <Cursor> cur = std::move (st.cursor);
		if (cur)
			cur->close ();
	}
}

bool PreparedStatement::execute ()
{
	check_exist ();
	return Statement::execute_first ();
}

}
