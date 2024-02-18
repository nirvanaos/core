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
#include "StatementBase.h"
#include "Cursor.h"

namespace SQLite {

Connection& StatementBase::connection ()
{
	if (!connection_)
		throw CORBA::OBJECT_NOT_EXIST ();
	if (connection_->isClosed ()) {
		close ();
		throw CORBA::OBJECT_NOT_EXIST ();
	}
	return *connection_;
}

void StatementBase::prepare (const IDL::String& sql, unsigned flags)
{
	finalize ();
	warnings_.clear ();

	const char* tail = sql.c_str ();
	const char* end = tail + sql.size ();
	Statements statements;
	try {
		for (;;) {
			sqlite3_stmt* stmt = nullptr;
			connection ().check_result (sqlite3_prepare_v3 (connection ().sqlite (), tail, (int)(end - tail), flags, &stmt, &tail));
			assert (stmt);
			statements.emplace_back (stmt);
			if (!*tail)
				break;
		}
	} catch (...) {
		while (!statements.empty ()) {
			sqlite3_finalize (statements.back ());
			statements.pop_back ();
		}
		throw;
	}
	statements_ = std::move (statements);
	cur_statement_ = statements_.size ();
}

void StatementBase::finalize () noexcept
{
	Connection& conn = connection ();
	Statements statements (std::move (statements_));
	cur_statement_ = 0;
	change_version ();
	for (auto stmt : statements) {
		conn.check_warning (sqlite3_finalize (stmt));
	}
}

bool StatementBase::execute_next ()
{
	assert (cur_statement_ < statements_.size ());

	result_set_ = nullptr;
	changed_rows_ = 0;

	sqlite3_stmt* stmt = statements_ [cur_statement_++];
	int step_result = step (stmt);
	changed_rows_ = sqlite3_changes (connection ().sqlite ());

	if (SQLITE_ROW == step_result || sqlite3_column_count (stmt) > 0) {
		NDBC::Rows prefetch;
		uint32_t cnt = 0;
		uint32_t size = 0;
		while (SQLITE_ROW == step_result) {
			uint32_t cb;
			prefetch.push_back (Cursor::get_row (stmt, cb));
			++cnt;
			size += cb;
			if (cnt >= prefetch_max_count_ || size >= prefetch_max_size_)
				break;
			step_result = step (stmt);
		}
		NDBC::Cursor::_ref_type cursor = CORBA::make_reference <Cursor> (
			std::ref (*this), stmt, (uint32_t)prefetch.size ())->_this ();
		result_set_ = NDBC::ResultSet::_factory->create (std::move (cursor), std::move (prefetch), page_max_count_, page_max_size_);
		return true;
	} else
		return false;
}

int StatementBase::step (sqlite3_stmt* stmt)
{
	auto ver = version ();

	int step_result = sqlite3_step (stmt);

	if (ver != version ())
		throw CORBA::INVALID_TRANSACTION (); // TODO: Maybe other exception?

	switch (step_result) {
	case SQLITE_ROW:
	case SQLITE_DONE:
		break;

	default:
		if (connection_)
			connection_->check_result (step_result);
	}

	return step_result;
}

}
