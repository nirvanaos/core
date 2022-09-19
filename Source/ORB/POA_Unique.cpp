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

POA_Unique::ReferencePtr POA_Unique::find_servant (const ProxyObject& proxy) NIRVANA_NOEXCEPT
{
	auto it = servant_map_.find (&proxy);
	if (it != servant_map_.end ())
		return it->second;
	else
		return nullptr;
}

void POA_Unique::activate_object (ReferenceLocal& ref, ProxyObject& proxy, unsigned flags)
{
	auto ins = servant_map_.emplace (&proxy, &ref);
	if (!ins.second)
		throw ServantAlreadyActive ();
	try {
		Base::activate_object (ref, proxy, flags);
	} catch (...) {
		servant_map_.erase (ins.first);
		throw;
	}
}

servant_reference <ProxyObject> POA_Unique::deactivate_object (ReferenceLocal& ref)
{
	servant_reference <ProxyObject> p (Base::deactivate_object (ref));
	servant_map_.erase (p);
	return p;
}

void POA_Unique::implicit_deactivate (ReferenceLocal& ref, ProxyObject& proxy) NIRVANA_NOEXCEPT
{
	servant_map_.erase (&proxy);
	Base::implicit_deactivate (ref, proxy);
}

ObjectId POA_Unique::servant_to_id (ProxyObject& proxy)
{
	ReferencePtr ref = find_servant (proxy);
	if (ref)
		return ref->object_id ();
	return servant_to_id_default (proxy, true);
}

Object::_ref_type POA_Unique::servant_to_reference (CORBA::Core::ProxyObject& proxy)
{
	ReferencePtr ref = find_servant (proxy);
	if (ref)
		return ref->get_proxy ();
	return servant_to_reference_default (proxy, true);
}

void POA_Unique::destroy_internal (bool etherealize_objects) NIRVANA_NOEXCEPT
{
	Base::destroy_internal (etherealize_objects);
	servant_map_.clear ();
}

void POA_Unique::etherealize_objects () NIRVANA_NOEXCEPT
{
	Base::etherealize_objects ();
	servant_map_.clear ();
}

}
}
