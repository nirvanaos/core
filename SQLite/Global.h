/*
* Nirvana SQLite module.
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
#ifndef SQLITE_GLOBAL_H_
#define SQLITE_GLOBAL_H_
#pragma once

#include "Driver.h"
#include "sqlite/sqlite3.h"

namespace SQLite {

/// Module global data
/// 
class Global
{
public:
	Global () :
		driver_ (CORBA::make_reference <Driver> ())
	{
		sqlite3_initialize ();
	}

	~Global ()
	{
		sqlite3_shutdown ();
	}
	
	Driver& driver () const noexcept
	{
		return *driver_;
	}

private:
	const CORBA::servant_reference <Driver> driver_;
};

extern const Global global;

}

#endif
