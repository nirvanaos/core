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
#include "pch.h"
#include "Driver.h"

namespace SQLite {

class DriverFactory;

}

namespace CORBA {
namespace Internal {

template <>
const char StaticId <SQLite::DriverFactory>::id [] = "NDBC/factory_sqlite";

}
}

namespace SQLite {

class DriverFactory :
	public CORBA::servant_traits <NDBC::DriverFactory>::ServantStatic <DriverFactory>
{
public:
	static NDBC::Driver::_ref_type getDriver ()
	{
		return CORBA::make_stateless <Driver> ()->_this ();
	}
};

}

NIRVANA_EXPORT_OBJECT (_exp_SQLite_factory, SQLite::DriverFactory)

