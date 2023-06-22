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
#include "Nirvana_policies.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace Nirvana::Core;

namespace PortableServer {
namespace Core {

POA_Retain::POA_Retain () :
	DGC_policy_ (DGC_DEFAULT)
{
	if (object_policies_) {
		bool gc;
		if (object_policies_->get_value <Nirvana::DGC_POLICY_TYPE> (gc)) {
			if (gc)
				DGC_policy_ = DGC_ENABLED;
			else {
				DGC_policy_ = DGC_DISABLED;
				object_policies_->erase (Nirvana::DGC_POLICY_TYPE);
			}
		}
	}
}

ReferenceLocalRef POA_Retain::activate_object (ObjectKey&& key, bool unique,
	ServantProxyObject& proxy, unsigned flags)
{
	ReferenceLocalRef ref = root ().emplace_reference (std::move (key), unique,
		std::ref (proxy), get_flags (flags), get_policies (flags));
	if (ref)
		activate_object (*ref, proxy);
	return ref;
}

void POA_Retain::activate_object (ReferenceLocal& ref, ServantProxyObject& proxy)
{
	ref.activate (proxy);
	references_.insert (&ref);
}

void POA_Retain::deactivate_object (ObjectId& oid)
{
	ReferenceLocalRef ref = root ().find_reference (IOP::ObjectKey (ObjectKey (*this, std::move (oid))));
	if (!ref)
		throw ObjectNotActive ();
	deactivate_object (*ref);
}

servant_reference <CORBA::Core::ServantProxyObject> POA_Retain::deactivate_object (ReferenceLocal& ref)
{
	servant_reference <ServantProxyObject> ret = ref.deactivate ();
	if (!ret)
		throw ObjectNotActive ();
	references_.erase (&ref);
	etherialize (ref.core_key ().object_id (), *ret, false);
	return ret;
}

unsigned POA_Retain::get_flags (unsigned flags) const noexcept
{
	switch (DGC_policy_) {
	case DGC_ENABLED:
		flags |= CORBA::Core::Reference::GARBAGE_COLLECTION;
		break;

	case DGC_DISABLED:
		flags &= ~CORBA::Core::Reference::GARBAGE_COLLECTION;
		break;
	}
	return flags;
}

void POA_Retain::implicit_deactivate (ReferenceLocal& ref, ServantProxyObject& proxy) noexcept
{
	references_.erase (&ref);
}

Object::_ref_type POA_Retain::reference_to_servant (Object::_ptr_type reference)
{
	if (!reference)
		throw BAD_PARAM ();

	ReferenceLocalRef ref = ProxyManager::cast (reference)->get_local_reference (*this);
	if (!ref)
		throw WrongAdapter ();
	servant_reference <ServantProxyObject> servant = ref->get_active_servant ();
	if (servant)
		return servant->get_proxy ();

	return reference_to_servant_default (true);
}

Object::_ref_type POA_Retain::id_to_servant (ObjectId& oid)
{
	ReferenceLocalRef ref = root ().find_reference (IOP::ObjectKey (ObjectKey (*this, std::move (oid))));
	if (ref) {
		servant_reference <ServantProxyObject> servant = ref->get_active_servant ();
		if (servant)
			return servant->get_proxy ();
	}
	return id_to_servant_default (true);
}

Object::_ref_type POA_Retain::id_to_reference (ObjectId& oid)
{
	ReferenceLocalRef ref = root ().find_reference (IOP::ObjectKey (ObjectKey (*this, std::move (oid))));
	if (ref && ref->get_active_servant ())
		return ref->get_proxy ();
	throw ObjectNotActive ();
}

void POA_Retain::destroy_internal (bool etherealize_objects) noexcept
{
	POA_Base::destroy_internal (etherealize_objects);
	if (etherealize_objects)
		POA_Retain::etherealize_objects ();
	else {
		References tmp (std::move (references_));
		for (auto p : tmp) {
			ReferenceLocalRef (p)->deactivate ();
		}
	}
}

void POA_Retain::etherealize_objects () noexcept
{
	References tmp (std::move (references_));
	for (auto p : tmp) {
		ReferenceLocalRef ref (p);
		servant_reference <ServantProxyObject> servant (ref->deactivate ());
		if (servant)
			etherialize (ref->core_key ().object_id (), *servant, true);
	}
}

void POA_Retain::serve_request (const RequestRef& request, ReferenceLocal& reference)
{
	servant_reference <ServantProxyObject> servant = reference.get_active_servant ();
	if (servant)
		POA_Base::serve_request (request, reference, *servant);
	else
		serve_default (request, reference);
}

}
}
