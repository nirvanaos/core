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
#ifndef NIRVANA_ORB_CORE_SERVANTBASE_H_
#define NIRVANA_ORB_CORE_SERVANTBASE_H_
#pragma once

#include "CoreServant.h"
#include "ServantProxyObject.h"
#include "Services.h"

namespace PortableServer {
namespace Core {

/// \brief Core implementation of ServantBase default operations.
class ServantBase :
	public CORBA::Core::CoreServant <ServantBase, CORBA::Core::ServantProxyObject>
{
	typedef CORBA::Core::CoreServant <ServantBase, CORBA::Core::ServantProxyObject> Base;

public:
	static ServantBase* create (Servant user_servant);

	// ServantBase default implementation

	Servant _core_servant () NIRVANA_NOEXCEPT
	{
		return &static_cast <PortableServer::ServantBase&> (
			static_cast <CORBA::Internal::Bridge <PortableServer::ServantBase>&> (*this));
	}

	static POA::_ref_type _default_POA ()
	{
		return POA::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::RootPOA));
	}

	CORBA::InterfaceDef::_ref_type _get_interface ()
	{
		return proxy ()._get_interface ();
	}

	bool _is_a (const IDL::String& type_id)
	{
		return proxy ()._is_a (type_id);
	}

	static bool _non_existent ()
	{
		return false;
	}

protected:
	ServantBase (CORBA::Core::ServantProxyObject& proxy) :
		Base (proxy)
	{}
};

}
}

#endif
