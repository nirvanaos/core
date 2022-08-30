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
#include "POA_Retain.h"
#include "RequestInBase.h"

using namespace CORBA;
using Nirvana::Core::ExecDomain;

namespace PortableServer {
namespace Core {

void POA_Retain::destroy (bool etherealize_objects, bool wait_for_completion)
{
	Base::destroy (etherealize_objects, wait_for_completion);
	active_object_map_.clear ();
}

void POA_Retain::deactivate_object (const ObjectId& oid)
{
	if (!active_object_map_.erase (oid))
		throw ObjectNotActive ();
}

Object::_ref_type POA_Retain::id_to_reference (const ObjectId& oid)
{
	AOM::const_iterator it = active_object_map_.find (oid);
	if (it != active_object_map_.end ())
		return it->second;

	throw ObjectNotActive ();
}

void POA_Retain::serve (CORBA::Core::RequestInBase& request) const
{
	auto it = active_object_map_.find (request.object_key ().object_id);
	if (it == active_object_map_.end ())
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
	Nirvana::Core::CoreRef <CORBA::Core::ProxyObject> proxy = CORBA::Core::object2proxy (it->second);
	ExecDomain::current ().leave_sync_domain ();
	request.serve_request (*proxy);
}

}
}
