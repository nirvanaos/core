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
			SQLITE_OPEN_URI | SQLITE_OPEN_EXRESCODE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
			nullptr);

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
		close ();
	}

	void close () noexcept;

	bool isClosed () const noexcept
	{
		return !sqlite_;
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

class Connection :
	public CORBA::servant_traits <NDBC::Connection>::Servant <Connection>,
	public SQLite
{
public:
	class Lock
	{
	public:
		Lock (Connection& conn);

		~Lock ()
		{
			conn_.busy_ = false;
		}

	private:
		Connection& conn_;
	};

	Connection (const std::string& uri) :
		SQLite (uri),
		busy_ (false),
		savepoint_ (0)
	{}

	void clearWarnings ()
	{
		check_exist ();
		warnings_.clear ();
	}

	void close ()
	{
		if (!isClosed ()) {
			{
				Lock lock (*this);
				SQLite::close ();
			}
			deactivate_servant (this);
		}
	}

	void commit ()
	{
		exec ("END");
	}

	NDBC::Statement::_ref_type createStatement (NDBC::ResultSet::Type resultSetType);

	bool getAutoCommit () const
	{
		check_ready ();
		return sqlite3_get_autocommit (*this);
	}

	static IDL::String getCatalog () noexcept
	{
		return IDL::String ();
	}

	IDL::String getSchema () const
	{
		check_ready ();
		return sqlite3_db_name (*this, 0);
	}

	TransactionIsolation getTransactionIsolation () const noexcept
	{
		return TransactionIsolation::TRANSACTION_SERIALIZABLE;
	}

	NDBC::SQLWarnings getWarnings () const
	{
		return warnings_;
	}

	bool isReadOnly () const noexcept
	{
		check_exist ();
		return sqlite3_db_readonly (*this, nullptr);
	}

	bool isValid () const noexcept
	{
		return !isClosed () && !busy_;
	}

	NDBC::PreparedStatement::_ref_type prepareCall (const IDL::String& sql, NDBC::ResultSet::Type resultSetType)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	NDBC::PreparedStatement::_ref_type prepareStatement (const IDL::String& sql, NDBC::ResultSet::Type resultSetType, unsigned flags);

	void releaseSavepoint (const NDBC::Savepoint& savepoint)
	{
		exec (("RELEASE " + savepoint).c_str ());
	}

	void rollback (const NDBC::Savepoint& savepoint)
	{
		if (savepoint.empty ())
			exec ("ROLLBACK");
		else
			exec (("ROLLBACK TO " + savepoint).c_str ());
	}

	void setAutoCommit (bool on)
	{
		bool current = getAutoCommit ();
		if (on) {
			if (!current)
				commit ();
		} else if (current)
			exec ("BEGIN");
	}

	static void setCatalog (const IDL::String&)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void setReadOnly (bool read_only)
	{
		if (isReadOnly () != read_only)
			throw CORBA::NO_PERMISSION ();
	}

	NDBC::Savepoint setSavepoint (IDL::String& name)
	{
		NDBC::Savepoint sp = std::move (name);
		if (sp.empty ())
			sp = std::to_string (++savepoint_);
		exec (("SAVEPOINT " + sp).c_str ());
		return sp;
	}

	void setSchema (const IDL::String&)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void setTransactionIsolation (int level)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	void check_warning (int err) noexcept;

private:
	void exec (const char* sql);

	void check_exist () const;
	void check_ready () const;

private:
	NDBC::SQLWarnings warnings_;
	bool busy_;
	unsigned savepoint_;
};

}

#endif
