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

#include "Connection.h"

namespace SQLite {

class Connection;

class PreparedStatement :
	public CORBA::servant_traits <NDBC::PreparedStatement>::Servant <PreparedStatement>
{
public:
	PreparedStatement (Connection& conn, const IDL::String& sql, unsigned flags) :
		connection_ (&conn)
	{
		const char* tail;
		connection_->check_result (sqlite3_prepare_v3 (connection_->sqlite (), sql.c_str (), (int)sql.size (), flags & SQLITE_PREPARE_PERSISTENT, &stmt_, &tail));
		if (*tail)
			connection_->add_warning (IDL::String ("SQL code not used: ") + tail);
	}

	~PreparedStatement ()
	{
		if (stmt_)
			connection_->check_warning (sqlite3_finalize (stmt_));
	}

	void check_exist () const;

	void close ()
	{
		check_exist ();
		connection_->check_result (sqlite3_finalize (stmt_));
		stmt_ = nullptr;
		deactivate_servant (this);
	}

	void setInt (unsigned i, long v)
	{
		check_exist ();
		connection_->check_result (sqlite3_bind_int (stmt_, i, v));
	}

	void setIntByName (const IDL::String& name, long v)
	{
		connection_->check_result (sqlite3_bind_int (stmt_, get_param_index (name), v));
	}

	unsigned get_param_index (const IDL::String& name) const;

private:
	CORBA::servant_reference <Connection> connection_;
	sqlite3_stmt* stmt_;
};

NDBC::PreparedStatement::_ref_type Connection::prepareStatement (const IDL::String& sql, unsigned flags)
{
	return CORBA::make_reference <PreparedStatement> (std::ref (*this), sql, flags)->_this ();
}

}

#endif
