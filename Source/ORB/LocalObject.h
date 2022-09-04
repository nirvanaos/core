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
#ifndef NIRVANA_ORB_CORE_LOCALOBJECT_H_
#define NIRVANA_ORB_CORE_LOCALOBJECT_H_
#pragma once

#include "CoreImpl.h"
#include "ProxyLocal.h"

namespace CORBA {
namespace Core {

/// \brief Core implementation of LocalObject default operations.
class LocalObject :
	public CoreServant <LocalObject, ProxyLocal>
{
	typedef CoreServant <LocalObject, ProxyLocal> Base;
public:
	static LocalObject* create (CORBA::LocalObject::_ptr_type user_servant);

	// LocalObject default implementation

	static Boolean _non_existent () NIRVANA_NOEXCEPT
	{
		return false;
	}

	Bridge <Object>* _get_object (Internal::Type <IDL::String>::ABI_in iid, Interface* env) NIRVANA_NOEXCEPT
	{
		return proxy ()._get_object (iid, env);
	}

protected:
	LocalObject (ProxyLocal& proxy) :
		Base (proxy)
	{}
};

}
}

#endif
