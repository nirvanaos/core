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
	Cursor (StatementBase& parent, sqlite3_stmt* stmt) :
		parent_ (&parent),
		parent_version_ (parent.version ()),
		stmt_ (stmt),
		position_ (0),
		after_end_ (false)
	{
		NIRVANA_TRACE ("SQLite: Cursor created");
	}

	~Cursor ()
	{
		NIRVANA_TRACE ("SQLite: Cursor destructed");
	}

	NDBC::RowIdx fetch (NDBC::RowOff pos, NDBC::Row& row)
	{
		Connection::Lock lock (check_parent ());

		if (pos)
			throw CORBA::BAD_PARAM ();

		if (after_end_)
			return position_;

		step ();
		
		if (!after_end_)
			row = get_row (stmt_);

		return position_;
	}

	NDBC::ColumnNames getColumnNames ()
	{
		Connection::Lock lock (check_parent ());

		NDBC::ColumnNames column_names;
		int cnt = sqlite3_column_count (stmt_);
		for (int i = 0; i < cnt; ++i) {
			column_names.emplace_back (sqlite3_column_name (stmt_, i));
		}

		return column_names;
	}

	void close ()
	{
		if (parent_)
			close_no_check ();
	}

	static NDBC::Row get_row (sqlite3_stmt* stmt);

private:
	Connection& check_parent ();

	void close_no_check () noexcept
	{
		parent_ = nullptr;
		// Do not call deactivate_servant for cursors to avoid
		// raising OBJECT_NOT_EXIST exception in rowset navigation ops.
	}

	void step ()
	{
		int step_result = sqlite3_step (stmt_);
		switch (step_result) {
		case SQLITE_ROW:
			++position_;
			break;
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
	const StatementBase::Version parent_version_;
	sqlite3_stmt* stmt_;
	uint32_t position_;
	bool after_end_;
};

}

#endif
