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
#ifndef SQLITE_DRIVER_H_
#define SQLITE_DRIVER_H_
#pragma once

#include "Global.h"
#include <Nirvana/NDBC_s.h>
#include <fnctl.h>

namespace SQLite {

class Driver : public CORBA::servant_traits <NDBC::Driver>::Servant <Driver>
{
public:
	Driver ()
	{}

	NDBC::DataSource::_ref_type getDataSource (const IDL::String& url) const
	{
		// Create file if not exists
		Nirvana::File::_ref_type file = global.open_file (url, O_CREAT)->file ();

		// Get file ID and create reference
		return NDBC::DataSource::_narrow (
			global.adapter ()->create_reference_with_id (file->id (), NDBC::_tc_DataSource->id ()));
	}

};

}

#endif
