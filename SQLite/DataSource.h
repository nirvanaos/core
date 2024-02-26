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

#include "Global.h"
#include "Connection_impl.h"
#include "filesystem.h"
#include <Nirvana/System.h>

namespace SQLite {

class DataSource : public CORBA::servant_traits <NDBC::DataSource>::Servant <DataSource>
{
public:
	DataSource (const PortableServer::ObjectId & id) :
		file_ (Nirvana::File::_narrow (global.file_system ()->get_item (id))),
		tuned_ (false)
	{
		if (!file_)
			throw CORBA::OBJECT_NOT_EXIST ();
		file_name_ = id_to_string (id);
		Nirvana::g_system->srand ((uint32_t)(uintptr_t)this);
	}

	~DataSource ()
	{}

	NDBC::Connection::_ref_type getConnection (const NDBC::Properties& props)
	{
		if (!tuned_) {
			tuned_ = true;
			if (!file_->size ()) {
				SQLite conn (file_name_);
				conn.exec ("PRAGMA journal_mode=WAL;");
			}
		}

		if (!props.empty ()) {
			std::string uri = "file:" + file_name_;
			auto it = props.begin ();
			uri += '?';
			uri += it->name ();
			uri += '=';
			uri += it->value ();
			while (props.end () != ++it) {
				uri += '&';
				uri += it->name ();
				uri += '=';
				uri += it->value ();
			}
			return CORBA::make_reference <Connection> (_this (), uri)->_this ();
		} else
			return CORBA::make_reference <Connection> (_this (), file_name_)->_this ();
	}

private:
	Nirvana::File::_ref_type file_; // Keep reference to prevent DGC
	std::string file_name_;
	bool tuned_;
};

}

#endif
