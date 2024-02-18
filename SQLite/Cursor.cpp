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
#include "Cursor.h"

namespace SQLite {

NDBC::Row Cursor::get_row (sqlite3_stmt* stmt, uint32_t& size)
{
	NDBC::Row row;
	size_t cb = 0;

	int cnt = sqlite3_data_count (stmt);
	assert (cnt > 0);
	for (int i = 0; i < cnt; ++i) {
		NDBC::Variant v;
		int ct = sqlite3_column_type (stmt, i);
		switch (ct) {
		case SQLITE_INTEGER:
			v.ll_val (sqlite3_column_int64 (stmt, i));
			cb += 8;
			break;
		case SQLITE_FLOAT:
			v.dbl_val (sqlite3_column_double (stmt, i));
			cb += 8;
			break;
		case SQLITE_TEXT: {
			size_t size = sqlite3_column_bytes (stmt, i);
			v.s_val (IDL::String ((const char*)sqlite3_column_text (stmt, i), size));
			cb += size;
		} break;
		case SQLITE_BLOB: {
			size_t size = sqlite3_column_bytes (stmt, i);
			const CORBA::Octet* b = (const CORBA::Octet*)sqlite3_column_blob (stmt, i);
			v.blob (NDBC::BLOB (b, b + size));
			cb += size;
		} break;
		}

		row.push_back (std::move (v));
	}

	size = (uint32_t)cb;
	return row;
}

void Cursor::check_exist ()
{
	if (!parent_)
		throw CORBA::OBJECT_NOT_EXIST ();
	try {
		if (parent_version_ != parent_->version ())
			throw CORBA::OBJECT_NOT_EXIST ();
	} catch (...) {
		parent_ = nullptr;
		deactivate_servant (this);
		throw;
	}
}

void Cursor::step ()
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
		error_ = step_result;
		parent_->connection ().check_result (step_result);
		break;
	}
}

}
