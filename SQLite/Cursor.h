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
	Cursor (StatementBase& parent, sqlite3_stmt* stmt, uint32_t position) :
		parent_ (&parent),
		parent_version_ (parent.version ()),
		stmt_ (stmt),
		position_ (position ? position : 1),
		after_end_ (!position),
		error_ (0)
	{}

	void check_exist ();

	NDBC::Rows getNext (uint32_t from, uint32_t max_cnt, uint32_t max_size)
	{
		if (!from)
			throw CORBA::BAD_PARAM ();

		check_exist ();

		if (from < position_) {
			parent_->connection ().check_result (sqlite3_reset (stmt_));
			position_ = 0;
			after_end_ = false;
			error_ = 0;
		}

		while (!after_end_ && !error_ && position_ <= from) {
			step ();
		}
		
		NDBC::Rows rows;
		uint32_t size = 0;
		while (!after_end_ && !error_ && rows.size () < max_cnt && size < max_size) {
			uint32_t cb;
			rows.push_back (get_row (stmt_, cb));
			size += cb;
			step ();
		}

		return rows;
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

	static NDBC::Row get_row (sqlite3_stmt* stmt, uint32_t& size);

private:
	void step ();

private:
	CORBA::servant_reference <StatementBase> parent_;
	const StatementBase::Version parent_version_;
	sqlite3_stmt* stmt_;
	uint32_t position_;
	bool after_end_;
	int error_;
};

}

#endif
