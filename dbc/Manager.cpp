/*
* Database connection module.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include "ConnectionPoolImpl.h"

namespace NDBC {

class Static_the_manager :
	public CORBA::servant_traits <Manager>::ServantStatic <Static_the_manager>
{
public:
	static ConnectionPool::_ref_type createConnectionPool (Driver::_ptr_type driver,
		IDL::String& url, IDL::String& user, IDL::String& password, unsigned options)
	{
		return CORBA::make_reference <ConnectionPoolImpl> (driver, std::ref (url), std::ref (user),
			std::ref (password))->_this ();
	}
};

}

NIRVANA_EXPORT_OBJECT (_exp_NDBC_the_manager, NDBC::Static_the_manager)
