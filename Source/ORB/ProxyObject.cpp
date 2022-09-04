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
#include "ServantBase.h"

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

Object::_ptr_type servant2object (PortableServer::Servant servant) NIRVANA_NOEXCEPT
{
	if (servant) {
		PortableServer::Servant ps = servant->_core_servant ();
		return static_cast <PortableServer::Core::ServantBase*> (
			static_cast <CORBA::Internal::Bridge <PortableServer::ServantBase>*> (&ps))->proxy ().get_proxy ();
	} else
		return nullptr;
}

PortableServer::ServantBase::_ref_type object2servant (CORBA::Object::_ptr_type obj)
{
	if (obj) {
		const CORBA::Core::ProxyObject* proxy = object2proxy (obj);

		if (&proxy->sync_context () != &Nirvana::Core::SyncContext::current ())
			throw CORBA::OBJ_ADAPTER ();
		return proxy->servant ();
	}
	return nullptr;
}

ProxyObject::ProxyObject (PortableServer::Servant servant) :
	ServantProxyBase (servant)
{}

ProxyObject::ProxyObject (const ProxyObject& src) :
	ServantProxyBase (src)
{}

Boolean ProxyObject::non_existent ()
{
	return servant ()->_non_existent ();
}

void ProxyObject::marshal (StreamOut& out)
{
	ReferenceLocal::marshal (ProxyManager::primary_interface_id (), out);
}

}
}
