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

#include "CoreImpl.h"
#include "ProxyObject.h"

namespace PortableServer {
namespace Core {

/// \brief Core implementation of ServantBase default operations.
class ServantBase final :
	public CORBA::Internal::Core::CoreImpl <ServantBase, PortableServer::ServantBase,
		CORBA::Internal::Core::ProxyObject>
{
	typedef CORBA::Internal::Core::CoreImpl <ServantBase, PortableServer::ServantBase,
		CORBA::Internal::Core::ProxyObject> Base;
public:
	using Skeleton <ServantBase, PortableServer::ServantBase>::__get_interface;
	using Skeleton <ServantBase, PortableServer::ServantBase>::__is_a;

	ServantBase (Servant servant) :
		Base (servant)
	{}

	// ServantBase default implementation

	Servant _core_servant () NIRVANA_NOEXCEPT
	{
		return &static_cast <PortableServer::ServantBase&> (
			static_cast <CORBA::Internal::Bridge <PortableServer::ServantBase>&> (*this));
	}

	static PortableServer::POA::_ref_type _default_POA ();

	bool _non_existent () const
	{
		return false;
	}

	static PortableServer::POA* default_POA_;
};

inline
CORBA::Object::_ptr_type servant2object (Servant servant)
{
	Servant ps = servant->_core_servant ();
	Core::ServantBase* core_obj = static_cast <Core::ServantBase*> (&ps);
	return core_obj->get_proxy ();
}

/// Returns nil for objects from other domains and for local objects
inline
PortableServer::ServantBase::_ref_type object2servant (CORBA::Object::_ptr_type obj)
{
	CORBA::Internal::Environment _env;
	CORBA::Internal::I_ret <PortableServer::ServantBase> _ret = (obj->_epv ().epv.get_servant) (&obj, &_env);
	_env.check ();
	return _ret;
}

}
}

#endif
