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
#ifndef SQLITE_CONNECTION_H_
#define SQLITE_CONNECTION_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/NDBC_s.h>
#include "sqlite/sqlite3.h"
#include "filesystem.h"

namespace SQLite {

void deactivate_servant (PortableServer::Servant servant) noexcept;

NIRVANA_NORETURN void throw_exception (IDL::String msg, int code = 0);

NDBC::SQLException create_exception (sqlite3* conn, int err);
NDBC::SQLWarning create_warning (sqlite3* conn, int err);
void check_result (sqlite3* conn, int err);

class SQLite
{
public:
	SQLite (const SQLite&) = delete;
	SQLite& operator = (const SQLite&) = delete;

	SQLite (const std::string& uri) :
		sqlite_ (nullptr)
	{
		int err = sqlite3_open_v2 (uri.c_str (), &sqlite_,
			SQLITE_OPEN_URI | SQLITE_OPEN_EXRESCODE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			VFS_NAME);

		if (SQLITE_OK != err) {
			if (sqlite_) {
				NDBC::SQLException ex = create_exception (err);
				sqlite3_close_v2 (sqlite_);
				throw ex;
			} else
				throw_exception (sqlite3_errstr (err), err);
		}
	}

	~SQLite ()
	{
		sqlite3_close_v2 (sqlite_);
	}

	operator sqlite3* () const noexcept
	{
		return sqlite_;
	}

	void check_result (int err) const
	{
		::SQLite::check_result (sqlite_, err);
	}

	NDBC::SQLWarning create_warning (int err) const
	{
		return ::SQLite::create_warning (sqlite_, err);
	}

	NDBC::SQLException create_exception (int err) const
	{
		return ::SQLite::create_exception (sqlite_, err);
	}

	void exec (const char* sql) const
	{
		check_result (sqlite3_exec (sqlite_, sql, nullptr, nullptr, nullptr));
	}

private:
	sqlite3* sqlite_;
};

class Stmt
{
public:
	Stmt (const Stmt&) = delete;
	Stmt& operator = (const Stmt&) = delete;

	Stmt (const SQLite& conn, const char* sql, int len = -1, unsigned flags = 0,
		const char** tail = nullptr)
	{
		conn.check_result (sqlite3_prepare_v3 (conn, sql, len, flags, &stmt_, tail));
	}

	~Stmt ()
	{
		sqlite3_finalize (stmt_);
	}

	operator sqlite3_stmt* () const noexcept
	{
		return stmt_;
	}

private:
	sqlite3_stmt* stmt_;
};

class Connection :
	public CORBA::servant_traits <NDBC::Connection>::Servant <Connection>,
	public SQLite
{
public:
	Connection (NDBC::DataSource::_ref_type&& parent, const std::string& uri) :
		SQLite (uri),
		parent_ (std::move (parent))
	{}

	void abort ()
	{}

	void clearWarnings ()
	{
		warnings_.clear ();
	}

	void close ()
	{
		check_exist ();
		parent_ = nullptr;
		deactivate_servant (this);
	}

	void commit ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	NDBC::Statement::_ref_type createStatement (NDBC::ResultSet::RSType resultSetType);

	bool getAutoCommit () const
	{
		return false;
	}

	static IDL::String getCatalog () noexcept
	{
		return IDL::String ();
	}

	NDBC::DataSource::_ref_type getDataSource () const
	{
		check_exist ();
		return parent_;
	}

	IDL::String getSchema ()
	{
		check_exist ();
		return sqlite3_db_name (*this, 0);
	}

	int16_t getTransactionIsolation () const noexcept
	{
		return 0;
	}

	NDBC::SQLWarnings getWarnings () const
	{
		return warnings_;
	}

	bool isClosed () const noexcept
	{
		return !parent_;
	}

	bool isReadOnly () const noexcept
	{
		return false;
	}

	bool isValid () const noexcept
	{
		return (bool)parent_;
	}

	NDBC::CallableStatement::_ref_type prepareCall (const IDL::String& sql, NDBC::ResultSet::RSType resultSetType)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	NDBC::PreparedStatement::_ref_type prepareStatement (const IDL::String& sql, NDBC::ResultSet::RSType resultSetType, unsigned flags);

	void releaseSavepoint (const IDL::String& name)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void rollback (const IDL::String& name)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void setAutoCommit (bool on)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static void setCatalog (const IDL::String&)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void setReadOnly (bool)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	IDL::String setSavepoint (IDL::String& name)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void setSchema (const IDL::String&)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void setTransactionIsolation (int level)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void check_exist () const;

	void check_warning (int err) noexcept;

private:
	NDBC::DataSource::_ref_type parent_; // Keep DataSource reference active
	NDBC::SQLWarnings warnings_;
};

}

#endif
