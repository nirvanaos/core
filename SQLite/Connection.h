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

void deactivate_servant (PortableServer::Servant servant);

class Connection : public CORBA::servant_traits <NDBC::Connection>::Servant <Connection>
{
public:
	Connection (NDBC::DataSource::_ref_type&& parent, sqlite3* sqlite) :
		parent_ (parent),
		sqlite_ (sqlite)
	{}

	void abort ()
	{}

	bool autoCommit () const
	{
		return false;
	}

	void autoCommit (bool on)
	{}

	static IDL::String catalog () noexcept
	{
		return IDL::String ();
	}

	static void catalog (const IDL::String&)
	{}

	void clearWarnings ()
	{}

	void close ()
	{}

	void commit ()
	{}

	NDBC::Statement::_ref_type createStatement (int32_t resultSetType, int32_t resultSetConcurrency)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	NDBC::SQLWarnings getWarnings () const
	{
		return NDBC::SQLWarnings ();
	}

	int32_t holdability () const
	{
		return 0;
	}

	void holdability (int32_t)
	{}

	IDL::String schema ()
	{
		return IDL::String ();
	}
	
	void schema (const IDL::String&)
	{}

	bool isClosed () const noexcept
	{
		return false;
	}

	bool isReadOnly () const noexcept
	{
		return false;
	}

	void isReadOnly (bool)
	{}

	bool isValid () const noexcept
	{
		return true;
	}

	IDL::String nativeSQL (IDL::String& sql)
	{
		return sql;
	}

	NDBC::CallableStatement::_ref_type prepareCall (const IDL::String& sql)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	NDBC::PreparedStatement::_ref_type prepareStatement (const IDL::String& sql, unsigned flags);

	void rollback ()
	{}

	int16_t transactionIsolation () const noexcept
	{
		return 0;
	}

	void transactionIsolation (int)
	{}

	sqlite3* sqlite () const noexcept
	{
		return sqlite_;
	}

	void add_warning (IDL::String&& msg);
	void add_warnings (NDBC::SQLWarnings&& warnings);

	void check_warning (int err) noexcept;

	void check_result (int err) const;

	NDBC::SQLWarning create_warning (int err) const;

private:
	NDBC::DataSource::_ref_type parent_; // Keep DataSource reference active
	sqlite3* sqlite_;
	NDBC::SQLWarnings warnings_;
};

}

#endif
