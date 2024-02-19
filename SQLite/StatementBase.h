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

class StatementBase :
	public virtual CORBA::servant_traits <NDBC::StatementBase>::base_type
{
public:

	virtual void close () override
	{
		finalize ();
		if (connection_) {
			connection_ = nullptr;
			deactivate_servant (this);
		}
	}

	virtual void clearWarnings () override
	{
		check_exist ();
		warnings_.clear ();
	}

	virtual NDBC::Connection::_ref_type getConnection () override
	{
		return connection ()._this ();
	}

	virtual bool getMoreResults () override;

	virtual NDBC::ResultSet::_ref_type getResultSet () override
	{
		return std::move (result_set_);
	}

	virtual int32_t getUpdateCount () override
	{
		return changed_rows_;
	}

	virtual NDBC::SQLWarnings getWarnings () override
	{
		return warnings_;
	}

	Connection& connection ();

	void check_exist ()
	{
		connection ();
	}

	typedef unsigned Version;

	Version version ()
	{
		check_exist ();
		return version_;
	}

	void change_version () noexcept
	{
		++version_;
	}

protected:
	StatementBase (Connection& connection) :
		connection_ (&connection),
		cur_statement_ (0),
		version_ (0),
		changed_rows_ (0),
		prefetch_max_count_ (16),
		prefetch_max_size_ (65536),
		page_max_count_ (16),
		page_max_size_ (65536)
	{}

	~StatementBase ()
	{
		finalize ();
	}

	void prepare (const IDL::String& sql, unsigned flags);
	void finalize () noexcept;

	typedef std::vector <sqlite3_stmt*> Statements;

	const Statements& statements () const noexcept
	{
		return statements_;
	}

	bool execute_first ()
	{
		change_version ();
		assert (!statements ().empty ());
		cur_statement_ = 0;
		return execute_next ();
	}

	bool execute_next ();

	int step (sqlite3_stmt* stmt);

private:
	CORBA::servant_reference <Connection> connection_;
	Statements statements_;
	NDBC::SQLWarnings warnings_;
	size_t cur_statement_;
	Version version_;
	uint32_t changed_rows_;
	uint32_t prefetch_max_count_;
	uint32_t prefetch_max_size_;
	uint32_t page_max_count_;
	uint32_t page_max_size_;
	NDBC::ResultSet::_ref_type result_set_;
};

}

#endif
