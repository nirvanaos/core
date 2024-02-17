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
#ifndef SQLITE_CURSOR_H_
#define SQLITE_CURSOR_H_
#pragma once

#include "Connection.h"

namespace SQLite {

class Statement;

class Cursor : public CORBA::servant_traits <NDBC::Cursor>::Servant <Cursor>
{
public:
	Cursor (Connection& connection, sqlite3_stmt* stmt, bool floating_fields) :
		connection_ (&connection),
		stmt_ (stmt),
		position_ (1),
		floating_fields_ (floating_fields)
	{}

	NDBC::Records getNext (uint32_t from, uint32_t max_cnt)
	{
		if (!connection_)
			throw CORBA::OBJECT_NOT_EXIST ();
		throw CORBA::NO_IMPLEMENT ();
	}

	void close () noexcept
	{
		connection_ = nullptr;
	}

private:
	CORBA::servant_reference <Connection> connection_;
	sqlite3_stmt* stmt_;
	uint32_t position_;
	bool floating_fields_;
};

}

#endif
