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
#include "Connection_impl.h"
#include "filesystem.h"

namespace SQLite {

class DataSource : public CORBA::servant_traits <NDBC::DataSource>::Servant <DataSource>
{
public:
	DataSource (const PortableServer::ObjectId& id) :
		file_ (OBJID_PREFIX + id_to_string (id))
	{
		SQLite conn (file_);
		Stmt stmt (conn, "PRAGMA journal_mode=WAL;");
		sqlite3_step (stmt);
	}

	~DataSource ()
	{}

	NDBC::Connection::_ref_type getConnection (const NDBC::Properties& props)
	{
		if (!props.empty ()) {
			std::string uri = "file:" + file_;
			auto it = props.begin ();
			uri += '?';
			uri += *it;
			while (props.end () != ++it) {
				uri += '&';
				uri += *it;
			}
			return CORBA::make_reference <Connection> (_this (), uri)->_this ();
		} else
			return CORBA::make_reference <Connection> (_this (), file_)->_this ();
	}

private:
	std::string file_;
};

}

#endif
