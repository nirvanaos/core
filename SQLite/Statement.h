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
#ifndef SQLITE_STATEMENT_H_
#define SQLITE_STATEMENT_H_
#pragma once

#include "Cursor.h"

namespace SQLite {

class Statement :
	public virtual CORBA::servant_traits <NDBC::PreparedStatement>::base_type
{
public:
	Statement (Connection& connection) :
		connection_ (&connection),
		cur_statement_ (0),
		version_ (0),
		changed_rows_ (0)
	{}

	~Statement ()
	{
		finalize ();
	}

	virtual void close () override;
	virtual void clearWarnings () override;
	virtual bool getMoreResults () override;
	virtual NDBC::ResultSet::_ref_type getResultSet () override;
	virtual int32_t getUpdateCount () override;
	virtual NDBC::SQLWarnings getWarnings () override;

protected:
	Connection& connection () const;

	void prepare (const IDL::String& sql, unsigned flags);
	void check_exist () const;
	void finalize () noexcept;

	struct Stmt
	{
		sqlite3_stmt* stmt;
		CORBA::servant_reference <Cursor> cursor;
	};

	typedef std::vector <Stmt> Statements;

	const Statements& statements () const noexcept
	{
		return statements_;
	}

	bool execute_first ()
	{
		++version_;
		assert (!statements ().empty ());
		cur_statement_ = 0;
		return execute_next ();
	}

	bool execute_next ();

private:
	CORBA::servant_reference <Connection> connection_;
	Statements statements_;
	NDBC::SQLWarnings warnings_;
	size_t cur_statement_;
	unsigned version_;
	unsigned changed_rows_;
	NDBC::ResultSet::_ref_type result_set_;
};

}

#endif
