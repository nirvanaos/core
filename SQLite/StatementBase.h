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
#ifndef SQLITE_STATEMENTBASE_H_
#define SQLITE_STATEMENTBASE_H_
#pragma once

#include "Connection.h"

namespace SQLite {

class StatementBase
{
public:
	void close ()
	{
		if (connection_) {
			{
				Connection::Lock lock (*connection_, true);
				finalize ();
			}
			connection_ = nullptr;
			deactivate_servant (servant_);
		}
	}

	bool isClosed () const noexcept
	{
		return !connection_;
	}

	void clearWarnings ()
	{
		check_exist ();
		warnings_.clear ();
	}

	NDBC::Connection::_ref_type getConnection ()
	{
		return connection ()._this ();
	}

	bool getMoreResults ()
	{
		Connection::Lock lock (connection ());
		if (0 < cur_statement_ && cur_statement_ < statements_.size ())
			return execute_next (true);
		else {
			result_set_ = nullptr;
			changed_rows_ = -1;
			return false;
		}
	}

	NDBC::ResultSet::_ref_type getResultSet () noexcept
	{
		return std::move (result_set_);
	}

	static NDBC::ResultSet::Type getResultSetType () noexcept
	{
		return NDBC::ResultSet::Type::TYPE_FORWARD_ONLY;
	}

	int32_t getUpdateCount ()
	{
		return changed_rows_;
	}

	NDBC::SQLWarnings getWarnings ()
	{
		return warnings_;
	}

	Connection& connection ();

	void check_exist ()
	{
		connection ();
	}

	typedef unsigned Version;

	Version version () const noexcept
	{
		return version_;
	}

	void change_version () noexcept
	{
		++version_;
	}

protected:
	StatementBase (Connection& connection, PortableServer::Servant servant) :
		connection_ (&connection),
		servant_ (servant),
		cur_statement_ (0),
		changed_rows_ (-1),
		version_ (0)
	{
		NIRVANA_TRACE ("SQLite: Statement created\n");
	}

	~StatementBase ()
	{
		if (connection_) {
			finalize (true);
			connection_ = nullptr;
		}
		NIRVANA_TRACE ("SQLite: Statement desctructed\n");
	}

	void prepare (const IDL::String& sql, unsigned flags);

	typedef std::vector <sqlite3_stmt*> Statements;

	const Statements& statements () const noexcept
	{
		return statements_;
	}

	bool execute_first (bool resultset)
	{
		change_version ();
		assert (!statements ().empty ());
		cur_statement_ = 0;
		return execute_next (resultset);
	}

	void finalize (bool silent = false) noexcept;
	void reset () noexcept;
	void check_warning (int err) noexcept;
	uint32_t executeUpdate ();

private:
	bool execute_next (bool resultset);
	int step (sqlite3_stmt* stmt);

private:
	CORBA::servant_reference <Connection> connection_;
	PortableServer::Servant servant_;
	Statements statements_;
	size_t cur_statement_;
	NDBC::SQLWarnings warnings_;
	NDBC::ResultSet::_ref_type result_set_;
	int32_t changed_rows_;
	Version version_;
};

}

#endif
