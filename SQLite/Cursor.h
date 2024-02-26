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
#ifndef SQLITE_CURSOR_H_
#define SQLITE_CURSOR_H_
#pragma once

#include "StatementBase.h"

namespace SQLite {

class Cursor : public CORBA::servant_traits <NDBC::Cursor>::Servant <Cursor>
{
public:
	Cursor (StatementBase& parent, PortableServer::Servant statement_servant, sqlite3_stmt* stmt) :
		parent_ (&parent),
		statement_servant_ (statement_servant),
		parent_version_ (parent.version ()),
		stmt_ (stmt),
		position_ (0),
		after_end_ (false)
	{}

	void check_exist ();

	NDBC::RowIdx fetch (NDBC::RowOff pos, NDBC::Row& row)
	{
		if (pos)
			throw CORBA::BAD_PARAM ();

		check_exist ();

		if (after_end_)
			return position_;

		step ();
		
		if (!after_end_)
			row = get_row (stmt_);

		return position_;
	}

	NDBC::ColumnNames getColumnNames ()
	{
		check_exist ();

		NDBC::ColumnNames names;
		int cnt = sqlite3_column_count (stmt_);
		for (int i = 0; i < cnt; ++i) {
			names.emplace_back (sqlite3_column_name (stmt_, i));
		}

		return names;
	}

	NDBC::StatementBase::_ref_type getStatement ()
	{
		check_exist ();
		return statement_servant_->_core_servant ()->_query_interface <NDBC::StatementBase> ();
	}

	static NDBC::Row get_row (sqlite3_stmt* stmt);

private:
	void step ()
	{
		int step_result = sqlite3_step (stmt_);
		switch (step_result) {
		case SQLITE_ROW:
			++position_;
		case SQLITE_DONE:
			++position_;
			after_end_ = true;
			break;
		default:
			parent_->connection ().check_result (step_result);
			break;
		}
	}

	NIRVANA_NORETURN static void raise_closed ();

private:
	StatementBase* parent_;
	PortableServer::ServantBase::_ref_type statement_servant_;
	const StatementBase::Version parent_version_;
	sqlite3_stmt* stmt_;
	uint32_t position_;
	bool after_end_;
};

}

#endif
