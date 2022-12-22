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
#include "SysDomain.h"
#include "ORB/Services.h"
#include <CORBA/IOP.h>
#include <CORBA/Proxy/ProxyBase.h>
#include "ORB/ESIOP.h"
#include "ORB/RemoteReferences.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace CORBA::Internal;
using namespace PortableServer;

namespace Nirvana {
namespace Core {

Object::_ref_type create_SysDomain ()
{
	if (ESIOP::sys_domain_id () == ESIOP::current_domain_id ()) {
		POA::_ref_type adapter = POA::_narrow (Services::bind (Services::RootPOA));
		servant_reference <SysDomain> obj = make_reference <SysDomain> ();
		adapter->activate_object_with_id (ObjectId (), obj);
		return obj->_this ();
	} else {
		IOP::ObjectKey object_key;
		object_key.resize (8, 0);
		ReferenceRemote ref (RemoteReferences::singleton ().get_domain (ESIOP::sys_domain_id ()), object_key,
			IOP::TaggedProfileSeq (), CORBA::Internal::RepIdOf <Nirvana::SysDomain>::id, 0);
		IORequest::_ref_type rq = ref.create_request (ref.find_operation ("get_service"), 3);
		Type <IDL::String>::marshal_in (CORBA::Internal::StringView <char> ("SysDomain"), rq);
		rq->invoke ();
		ProxyRoot::check_request (rq);
		Nirvana::SysDomain::_ref_type obj;
		Type <Nirvana::SysDomain>::unmarshal (rq, obj);
		return obj;
	}
}

}
}

