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
#include "POA.h"
#include <CORBA/Proxy/TypeCodeNative.h>
#include "../Binder.inl"

using namespace CORBA::Internal;
using namespace CORBA;
using namespace Nirvana::Core;
using namespace Nirvana;

namespace PortableServer {
namespace Core {

POA* ServantBase::default_POA_ = nullptr;
std::atomic <int> ServantBase::next_timestamp_;

PortableServer::POA::_ref_type ServantBase::_default_POA ()
{
	if (!default_POA_) {
		Object::_ref_type svc = Binder::bind_service (CORBA::Internal::Core::Services::RootPOA);
		PortableServer::POA::_ref_type poa = PortableServer::POA::_narrow (svc);
		if (!poa)
			throw_INV_OBJREF ();
		default_POA_ = static_cast <PortableServer::POA*> (&(PortableServer::POA::_ptr_type)poa);
		return poa;
	} else
		return PortableServer::POA::_ptr_type (default_POA_);
}

typedef TypeCodeNative <PortableServer::ServantBase> TC_Servant;

Object::_ptr_type servant2object (Servant servant)
{
	if (servant) {
		Servant ps = servant->_core_servant ();
		Core::ServantBase* core_obj = static_cast <Core::ServantBase*> (
			static_cast <CORBA::Internal::Bridge <PortableServer::ServantBase>*> (&ps));
		return core_obj->get_proxy ();
	} else
		return nullptr;
}

PortableServer::ServantBase::_ref_type object2servant (CORBA::Object::_ptr_type obj)
{
	if (obj) {
		const Core::ServantBase& svt = *object2servant_core (obj);

		if (&svt.sync_context () != &Nirvana::Core::SyncContext::current ())
			throw CORBA::OBJ_ADAPTER ();
		return svt.servant ();
	}
	return nullptr;
}

}
}

namespace CORBA {
namespace Internal {
template <>
const Char TypeCodeName <PortableServer::ServantBase>::name_ [] = "Servant";
}
}

__declspec (selectany) const Nirvana::ImportInterfaceT <CORBA::TypeCode>
	PortableServer::_tc_Servant = { Nirvana::OLF_IMPORT_INTERFACE, nullptr, nullptr,
	NIRVANA_STATIC_BRIDGE (CORBA::TypeCode, PortableServer::Core::TC_Servant) };
