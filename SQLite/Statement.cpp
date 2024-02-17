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
#include "Statement.h"
#include "Cursor.h"

namespace SQLite {

void Statement::check_exist () const
{
	if (!connection_)
		throw CORBA::OBJECT_NOT_EXIST ();
}

Connection& Statement::connection () const
{
	if (!connection_)
		throw CORBA::OBJECT_NOT_EXIST ();
	return *connection_;
}

void Statement::prepare (const IDL::String& sql, unsigned flags)
{
	finalize ();
	warnings_.clear ();

	const char* tail = sql.c_str ();
	const char* end = tail + sql.size ();
	sqlite3_stmt* stmt = nullptr;
	Statements statements;
	try {
		for (;;) {
			connection ().check_result (sqlite3_prepare_v3 (connection ().sqlite (), tail, (int)(end - tail), flags, &stmt, &tail));
			assert (stmt);
			statements.emplace_back (stmt);
			if (!*tail)
				break;
			stmt = nullptr;
		}
	} catch (...) {
		while (!statements.empty ()) {
			sqlite3_finalize (statements.back ().stmt);
			statements.pop_back ();
		}
		throw;
	}
	statements_ = std::move (statements);
	cur_statement_ = statements_.size ();
}

void Statement::finalize () noexcept
{
	Connection& conn = connection ();
	Statements statements (std::move (statements_));
	cur_statement_ = 0;
	++version_;
	for (const auto& stmt : statements) {
		if (stmt.cursor)
			stmt.cursor->close ();
		conn.check_warning (sqlite3_finalize (stmt.stmt));
	}
}

void Statement::close ()
{
	finalize ();
	if (connection_) {
		connection_ = nullptr;
		deactivate_servant (this);
	}
}

void Statement::clearWarnings ()
{
	warnings_.clear ();
}

bool Statement::getMoreResults ()
{
	return cur_statement_ < statements_.size ();
}

NDBC::ResultSet::_ref_type Statement::getResultSet ()
{
	return nullptr;
}

int32_t Statement::getUpdateCount ()
{
	return 0;
}

NDBC::SQLWarnings Statement::getWarnings ()
{
	check_exist ();
	return warnings_;
}

bool Statement::execute_next ()
{
	assert (cur_statement_ < statements_.size ());
	Stmt& stmt_info = statements_ [cur_statement_++];

	result_set_ = nullptr;
	changed_rows_ = 0;

	auto version = version_;

	sqlite3_stmt* stmt = stmt_info.stmt;
	assert (!stmt_info.cursor);
	int step_result = sqlite3_step (stmt);
	check_exist ();
	if (version != version_)
		throw CORBA::INVALID_TRANSACTION (); // TODO: Maybe other exception?

	if (SQLITE_ERROR == step_result) {
		connection_->check_result (step_result);
		return false;
	} else if (SQLITE_ROW == step_result) {
		NDBC::Columns metadata;
		NDBC::FieldOffsets field_offsets;
		NDBC::Blob data;
		bool floating_fields = false;

		size_t field_offset = 0;
		int cnt = sqlite3_column_count (stmt);
		for (int i = 0; i < cnt; ++i) {
			NDBC::DataType dt = NDBC::DataType::DB_NULL;
			int ct = sqlite3_column_type (stmt, i);
			bool var_len = false;
			size_t field_len = 0;
			data.resize (field_offset);
			switch (ct) {
			case SQLITE_INTEGER: {
				dt = NDBC::DataType::DB_BIGINT;
				int64_t v = sqlite3_column_int64 (stmt, i);
				field_len = sizeof (v);
				data.insert (data.end (), (const uint8_t*)&v, (const uint8_t*)(&v + 1));
			} break;
			case SQLITE_FLOAT: {
				dt = NDBC::DataType::DB_DOUBLE;
				CORBA::Double v = sqlite3_column_double (stmt, i);
				field_len = sizeof (v);
				data.insert (data.end (), (const uint8_t*)&v, (const uint8_t*)(&v + 1));
			} break;
			case SQLITE_TEXT: {
				dt = NDBC::DataType::DB_VARCHAR;
				var_len = true;
				field_len = sqlite3_column_bytes (stmt, i);
				const uint8_t* p = (const uint8_t*)sqlite3_column_blob (stmt, i);
				data.insert (data.end (), p, p + field_len);
			} break;
			case SQLITE_BLOB: {
				dt = NDBC::DataType::DB_BINARY;
				var_len = true;
				field_len = sqlite3_column_bytes (stmt, i);
				const uint8_t* p = sqlite3_column_text (stmt, i);
				data.insert (data.end (), p, p + field_len);
			} break;
			}

			if (var_len)
				field_offset = Nirvana::round_up (field_offset, (size_t)4);
			else if (field_len) // not NULL
				field_offset = Nirvana::round_up (field_offset, (size_t)8);

			if (i > 0)
				field_offsets.push_back ((uint32_t)field_offset);

			if (var_len && (i + 1 < cnt))
				floating_fields = true;

			field_offset += field_len;

			const char* name = sqlite3_column_name (stmt, i);
			metadata.emplace_back (name, dt, NDBC::DataLength ());
		}

		if (floating_fields) {
			// Align to 8 bytes
			if (field_offsets.size () % 2)
				field_offsets.push_back (0);

			// Prefix record with field offsets
			data.insert (data.begin (), (const uint8_t*)field_offsets.data (), (const uint8_t*)(field_offsets.data () + field_offsets.size ()));

			// Field offsets is empty for floating fields
			field_offsets.clear ();
		}

		stmt_info.cursor = CORBA::make_reference <Cursor> (std::ref (connection ()), stmt, floating_fields);
		result_set_ = NDBC::ResultSet::_factory->create (std::move (metadata), std::move (field_offsets), std::move (data), stmt_info.cursor->_this ());
		return true;
	} else {
		changed_rows_ = sqlite3_changes (connection ().sqlite ());
		return false;
	}
}

}
