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
#include "ReferenceLocal.h"

using namespace Nirvana::Core;

namespace CORBA {
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

PortableServer::ServantBase::_ref_type object2servant (Object::_ptr_type obj)
{
	if (obj) {
		const CORBA::Core::ProxyObject* proxy = object2proxy (obj);

		if (&proxy->sync_context () != &Nirvana::Core::SyncContext::current ())
			throw CORBA::OBJ_ADAPTER ();
		return proxy->servant ();
	}
	return nullptr;
}

ProxyObject::ProxyObject (PortableServer::Core::ServantBase& core_servant, PortableServer::Servant user_servant) :
	ServantProxyBase (user_servant),
	core_servant_ (core_servant)
{}

Boolean ProxyObject::non_existent ()
{
	return servant ()->_non_existent ();
}

ReferenceLocalRef ProxyObject::get_reference_local () const NIRVANA_NOEXCEPT
{
	ReferenceLocalRef ref (reference_.lock ());
	reference_.unlock ();
	return ref;
}

ReferenceRef ProxyObject::get_reference ()
{
	ReferenceRef ref (get_reference_local ());
	if (!ref) // Attempt to pass an unactivated (unregistered) value as an object reference.
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
	return ref;
}

}
}
