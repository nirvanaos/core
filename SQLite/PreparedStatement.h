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
#ifndef SQLITE_PREPAREDSTATEMENT_H_
#define SQLITE_PREPAREDSTATEMENT_H_
#pragma once

#include "Statement.h"

namespace SQLite {

class PreparedStatement :
	public virtual CORBA::servant_traits <NDBC::PreparedStatement>::base_type,
	public virtual Statement
{
public:
	PreparedStatement (Connection& conn, const IDL::String& sql, unsigned flags) :
		Statement (conn)
	{
		prepare (sql, flags);
	}

	~PreparedStatement ()
	{}

	struct ParamIndex
	{
		sqlite3_stmt* stmt;
		unsigned idx;
	};

	virtual void clearParameters () override;
	
	virtual bool execute () override;

	virtual void setInt (uint16_t i, int32_t v) override
	{
		ParamIndex pi = get_param_index (i);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	virtual void setIntByName (const IDL::String& name, int32_t v) override
	{
		ParamIndex pi = get_param_index (name);
		connection ().check_result (sqlite3_bind_int (pi.stmt, pi.idx, v));
	}

	ParamIndex get_param_index (unsigned i) const;
	ParamIndex get_param_index (const IDL::String & name) const;

};

NDBC::PreparedStatement::_ref_type Connection::prepareStatement (const IDL::String& sql, unsigned flags)
{
	return CORBA::make_reference <PreparedStatement> (std::ref (*this), sql, flags)->_this ();
}

}

#endif
