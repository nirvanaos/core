/// \file
/*
* Nirvana Core.
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
#ifndef NIRVANA_CORE_DRIVERMANAGER_H_
#define NIRVANA_CORE_DRIVERMANAGER_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/NDBC_s.h>

namespace Nirvana {
namespace Core {

class DriverManager :
	public CORBA::servant_traits <NDBC::DriverManager>::Servant <DriverManager>
{
public:
	DriverManager ()
	{

	}

	NDBC::Connection::_ref_type getConnection (IDL::String& url, const IDL::String& user,
		const IDL::String& pwd)
	{
		size_t id_len = url.find (':');
		if (id_len == IDL::String::npos)
			raise ("Invalid url: " + url);
		std::string id = url.substr (0, id_len);
		std::transform (id.begin (), id.end (), id.begin (), tolower);
		if (id != "sqlite")
			raise ("Unknown driver: " + id);

		url.erase (0, id_len + 1);
		NDBC::Driver::_ref_type driver = sqlite_;
		NDBC::DataSource::_ref_type ds = driver->getDataSource (url);
		return ds->getConnection (user, pwd);
	}

private:
	NIRVANA_NORETURN static void raise (IDL::String msg);

private:
	NDBC::Driver::_ref_type sqlite_;
};

}
}

#endif
