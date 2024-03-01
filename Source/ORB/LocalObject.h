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

#include "CoreServant.h"
#include "ServantProxyLocal.h"

namespace CORBA {
namespace Core {

/// \brief Core implementation of LocalObject default operations.
class LocalObject :
	public CoreServant <LocalObject, ServantProxyLocal>
{
	typedef CoreServant <LocalObject, ServantProxyLocal> Base;

public:
	static LocalObject* create (CORBA::LocalObject::_ptr_type user_servant, Object::_ptr_type comp);

	// LocalObject default implementation

	static Boolean _non_existent () noexcept
	{
		return false;
	}
	
	Bridge <Object>* _get_object (IDL::Type <IDL::String>::ABI_in iid, Interface* env) noexcept
	{
		if (Internal::RepId::check (Internal::RepIdOf <Object>::id, IDL::Type <IDL::String>::in (iid)) != Internal::RepId::COMPATIBLE)
			set_INV_OBJREF (env);
		return static_cast <Bridge <Object>*> (&proxy ().get_proxy ());
	}

protected:
	LocalObject (ServantProxyLocal& proxy) :
		Base (proxy)
	{}
};

class LocalObjectImpl :
	public ServantProxyLocal,
	public LocalObject
{
public:
	LocalObjectImpl (CORBA::LocalObject::_ptr_type user_servant, Object::_ptr_type comp);
};

inline
LocalObject* LocalObject::create (CORBA::LocalObject::_ptr_type user_servant, Object::_ptr_type comp)
{
	return new LocalObjectImpl (user_servant, comp);
}

}
}

#endif
