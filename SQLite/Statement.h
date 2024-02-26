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
#ifndef SQLITE_STATEMENT_H_
#define SQLITE_STATEMENT_H_
#pragma once

#include "StatementBase.h"

namespace SQLite {

class Statement final :
	public StatementBase,
	public CORBA::servant_traits <NDBC::Statement>::Servant <Statement>
{
public:
	Statement (Connection& connection) :
		StatementBase (connection, this)
	{}

	~Statement ()
	{}

	bool execute (const IDL::String& sql)
	{
		check_exist ();
		prepare (sql, 0);
		return execute_first (true);
	}

	NDBC::ResultSet::_ref_type executeQuery (const IDL::String& sql)
	{
		execute (sql);
		return getResultSet ();
	}

	uint32_t executeUpdate (const IDL::String& sql)
	{
		check_exist ();
		prepare (sql, 0);
		uint32_t ret = StatementBase::executeUpdate ();
		finalize ();
		return ret;
	}

};

}

#endif
