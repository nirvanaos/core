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
#ifndef SQLITE_DATASOURCE_H_
#define SQLITE_DATASOURCE_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include "Connection.h"
#include "filesystem.h"

namespace SQLite {

class DataSource : public CORBA::servant_traits <NDBC::DataSource>::Servant <DataSource>
{
public:
	DataSource (const PortableServer::ObjectId& id) :
		sqlite_ (nullptr)
	{
		std::string file = OBJID_PREFIX + id_to_string (id);
		sqlite3_open_v2 (file.c_str (), &sqlite_, 0, VFS_NAME);
	}

	~DataSource ()
	{
		sqlite3_close_v2 (sqlite_);
	}

	NDBC::Connection::_ref_type getConnection (const IDL::String& user, const IDL::String& pwd)
	{
		return CORBA::make_reference <Connection> (_this (), sqlite_)->_this ();
	}

private:
	sqlite3* sqlite_;
};

}

#endif
