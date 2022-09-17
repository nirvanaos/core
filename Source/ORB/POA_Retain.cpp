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
#include <CORBA/Servant_var.h>

using namespace CORBA;
using namespace CORBA::Core;
using namespace Nirvana::Core;

namespace PortableServer {
namespace Core {

servant_reference <ReferenceLocal> POA_Retain::activate_object (ObjectKey&& key, ProxyObject& proxy, bool implicit)
{
	auto ins = root_->create_reference (std::move (key), std::ref (proxy.core_servant ()), implicit);
	if (!ins.second)
		throw ObjectAlreadyActive ();
	Servant_var <ReferenceLocal> ref (&const_cast <ReferenceLocal&> (*ins.first));
	proxy.activate (*ref);
	references_.insert (ref);
	return ref;
}

servant_reference <ProxyObject> POA_Retain::deactivate_object (const ObjectId& oid)
{
	ReferenceLocalRef ref = root_->find_reference (ObjectKey (*this, oid));
	if (!ref)
		throw ObjectNotActive ();
	servant_reference <ProxyObject> ret = ref->deactivate ();
	if (!ret)
		throw ObjectNotActive ();
	references_.erase (ref);
	etherialize (*ret);
	return ret;
}

void POA_Retain::implicit_deactivate (ReferenceLocal& ref, ProxyObject& proxy) NIRVANA_NOEXCEPT
{
	references_.erase (&ref);
}

Object::_ref_type POA_Retain::reference_to_servant (Object::_ptr_type reference)
{
	if (!reference)
		throw BAD_PARAM ();

	ReferenceRef ref = ProxyManager::cast (reference)->get_reference ();
	if (!ref)
		throw WrongAdapter ();
	ReferenceLocal* loc = ref->local_reference ();
	if (!loc)
		throw WrongAdapter ();
	if (!check_path (loc->adapter_path ()))
		throw WrongAdapter ();
	CoreRef <ProxyObject> servant = loc->get_servant ();
	if (servant)
		return servant->get_proxy ();

	return reference_to_servant_default (true);
}

Object::_ref_type POA_Retain::id_to_servant (const ObjectId& oid)
{
	ReferenceLocalRef ref = root_->find_reference (ObjectKey (*this, oid));
	if (ref) {
		CoreRef <ProxyObject> servant = ref->get_servant ();
		if (servant)
			return servant->get_proxy ();
	}
	return id_to_servant_default (true);
}

Object::_ref_type POA_Retain::id_to_reference (const ObjectId& oid)
{
	ReferenceLocalRef ref = root_->find_reference (ObjectKey (*this, oid));
	if (ref && ref->get_servant ())
		return ref->get_proxy ();
	throw ObjectNotActive ();
}

void POA_Retain::destroy_internal (bool etherealize_objects) NIRVANA_NOEXCEPT
{
	POA_Base::destroy_internal (etherealize_objects);
	if (etherealize_objects)
		POA_Retain::etherealize_objects ();
	else {
		References tmp (std::move (references_));
		for (void* p : tmp) {
			ReferenceLocalRef (reinterpret_cast <ReferenceLocal*> (p))->deactivate ();
		}
	}
}

void POA_Retain::etherealize_objects () NIRVANA_NOEXCEPT
{
	References tmp (std::move (references_));
	for (void* p : tmp) {
		servant_reference <ProxyObject> servant =
			ReferenceLocalRef (reinterpret_cast <ReferenceLocal*> (p))->deactivate ();
		if (servant)
			etherialize (*servant);
	}
}

void POA_Retain::serve (const RequestRef& request, ReferenceLocal& reference)
{
	CoreRef <ProxyObject> servant = reference.get_servant ();
	if (servant)
		POA_Base::serve (request, reference, *servant);
	serve_default (request, reference);
}

}
}
