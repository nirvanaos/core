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
#include "POA_Root.h"

using namespace Nirvana::Core;
using namespace CORBA;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

POA_Unique::ReferencePtr POA_Unique::find_servant (const ServantProxyObject& proxy) noexcept
{
	auto it = servant_map_.find (&proxy);
	if (it != servant_map_.end ())
		return it->second;
	else
		return nullptr;
}

void POA_Unique::activate_object (ReferenceLocal& ref, ServantProxyObject& proxy)
{
	auto ins = servant_map_.emplace (&proxy, &ref);
	if (!ins.second)
		throw ServantAlreadyActive ();
	try {
		Base::activate_object (ref, proxy);
	} catch (...) {
		servant_map_.erase (ins.first);
		throw;
	}
}

servant_reference <ServantProxyObject> POA_Unique::deactivate_reference (ReferenceLocal& ref,
	bool etherealize, bool cleanup_in_progress)
{
	servant_reference <ServantProxyObject> p (Base::deactivate_reference (ref, etherealize, cleanup_in_progress));
	servant_map_.erase (p);
	return p;
}

void POA_Unique::implicit_deactivate (ReferenceLocal& ref, ServantProxyObject& proxy) noexcept
{
	servant_map_.erase (&proxy);
	Base::implicit_deactivate (ref, proxy);
}

ObjectId POA_Unique::servant_to_id (ServantProxyObject& proxy)
{
	ReferencePtr ref = find_servant (proxy);
	if (ref)
		return ObjectKey::get_object_id (ref->object_key ());
	return servant_to_id_default (proxy, true);
}

Object::_ref_type POA_Unique::servant_to_reference (CORBA::Core::ServantProxyObject& proxy)
{
	ReferencePtr ref = find_servant (proxy);
	if (ref)
		return ref->get_proxy ();
	return servant_to_reference_default (proxy, true);
}

}
}
