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
#include <Nirvana/System.h>
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
	static const TimeBase::TimeT BUSY_TIMEOUT_MIN = TimeBase::MILLISECOND * 1;
	static const unsigned BUSY_TIMEOUT_POW_MAX = 10;
	static const unsigned BUSY_TRY_MAX = 20;

public:
	class Lock
	{
	public:
		Lock (Connection& conn, bool no_check_exist = false);

		~Lock ()
		{
			connection_.data_state_->consistent ();
		}

		Connection& connection () const noexcept
		{
			return connection_;
		}

	private:
		Connection& connection_;
	};

	Connection (const std::string& uri) :
		SQLite (uri),
		data_state_ (Nirvana::the_system->create_data_state ()),
		timeout_ (0),
		savepoint_ (0),
		last_busy_cnt_ (1),
		busy_timeout_pow_ (BUSY_TIMEOUT_POW_MAX / 4)
	{
		sqlite3_busy_handler (*this, s_busy_handler, this);
		sqlite3_filename fn = sqlite3_db_filename (*this, nullptr);
		if (fn) {
			const char* journal_mode = sqlite3_uri_parameter (fn, "journal_mode");
			if (journal_mode) {
				std::string cmd = "PRAGMA journal_mode=";
				cmd += journal_mode;
				cmd += ';';
				SQLite::exec (cmd.c_str ());
			}
		}
		NIRVANA_TRACE ("SQLite: Connection created\n");
	}

	~Connection ()
	{
		NIRVANA_TRACE ("SQLite: Connection destructed\n");
	}

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
		exec ("END;BEGIN");
	}

	NDBC::Statement::_ref_type createStatement (NDBC::ResultSet::Type resultSetType);

	bool getAutoCommit () const
	{
		check_exist ();
		return sqlite3_get_autocommit (*this);
	}

	void setAutoCommit (bool on)
	{
		bool current = sqlite3_get_autocommit (*this);
		if (on) {
			if (!current)
				exec ("END");
		} else if (current)
			exec ("BEGIN");
	}

	NDBC::SQLWarnings getWarnings () const
	{
		return warnings_;
	}

	void setTimeout (const TimeBase::TimeT& timeout) noexcept
	{
		timeout_ = timeout;
	}

	TimeBase::TimeT getTimeout () const noexcept
	{
		return timeout_;
	}

	NDBC::PreparedStatement::_ref_type prepareStatement (const IDL::String& sql, NDBC::ResultSet::Type resultSetType, unsigned flags);

	void releaseSavepoint (const NDBC::Savepoint& savepoint)
	{
		exec (("RELEASE " + savepoint).c_str ());
	}

	void rollback (const NDBC::Savepoint& savepoint)
	{
		if (savepoint.empty ())
			exec ("ROLLBACK;BEGIN");
		else
			exec (("ROLLBACK TO " + savepoint).c_str ());
	}

	static IDL::String getCatalog () noexcept
	{
		return IDL::String ();
	}

	static void setCatalog (const IDL::String& c)
	{
		if (!c.empty ())
			throw CORBA::NO_IMPLEMENT ();
	}

	bool isReadOnly () const noexcept
	{
		check_exist ();
		return sqlite3_db_readonly (*this, nullptr);
	}

	void setReadOnly (bool read_only) const
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

	IDL::String getSchema () const
	{
		check_exist ();
		return sqlite3_db_name (*this, 0);
	}

	void setSchema (const IDL::String& s)
	{
		if (s != sqlite3_db_name (*this, 0))
			throw CORBA::NO_IMPLEMENT ();
	}

	TransactionIsolation getTransactionIsolation () const noexcept
	{
		return TransactionIsolation::TRANSACTION_SERIALIZABLE;
	}

	void setTransactionIsolation (TransactionIsolation level)
	{
		if (level != TransactionIsolation::TRANSACTION_SERIALIZABLE)
			throw CORBA::NO_IMPLEMENT ();
	}

	void check_warning (int err) noexcept;

private:
	void exec (const char* sql);
	void check_exist () const;

	static int s_busy_handler (void*, int attempt) noexcept;
	inline int busy_handler (int attempt) noexcept;

private:
	NDBC::SQLWarnings warnings_;
	Nirvana::DataState::_ref_type data_state_;
	TimeBase::TimeT timeout_;
	unsigned savepoint_;
	unsigned last_busy_cnt_;
	unsigned busy_timeout_pow_;
};

}

#endif
