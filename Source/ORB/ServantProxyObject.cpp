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
#include "../pch.h"
#include "ServantProxyObject.h"
#include "ReferenceLocal.h"
#include "POA_Root.h"

using namespace Nirvana::Core;
using namespace PortableServer::Core;

namespace CORBA {
namespace Core {

Object::_ptr_type servant2object (PortableServer::Servant servant) noexcept
{
	if (servant) {
		PortableServer::Servant ps = servant->_core_servant ();
		return static_cast <PortableServer::Core::ServantBase*> (
			static_cast <CORBA::Internal::Bridge <PortableServer::ServantBase>*> (&ps))->proxy ().get_proxy ();
	} else
		return nullptr;
}

PortableServer::Servant object2servant (Object::_ptr_type obj)
{
	if (obj)
		return proxy2servant (object2proxy (obj));
	return nullptr;
}

PortableServer::Servant proxy2servant (ServantProxyObject* proxy)
{
	if (proxy) {
		if (&proxy->sync_context () != &Nirvana::Core::SyncContext::current ())
			throw OBJ_ADAPTER ();
		return proxy->servant ();
	}
	return nullptr;
}

ServantProxyObject::ServantProxyObject (PortableServer::Servant servant, Internal::String_in interface_id) :
	ServantProxyBase (servant, interface_id),
	adapter_context_ (&local2proxy (Services::bind (Services::RootPOA))->sync_context ()),
	references_ (adapter_context_->sync_domain ()->mem_context ().heap ())
{}

void ServantProxyObject::_add_ref ()
{
	static_assert (sizeof (ReferenceLocal) == REF_SIZE, "sizeof (ReferenceLocal)");

	RefCntProxy::IntegralType cnt = ServantProxyBase::add_ref ();

	try {
		if (1 == cnt && servant_) {
			if (!reference_.load ()) {
				PortableServer::POA::_ref_type adapter = servant ()->_default_POA ();
				if (adapter && POA_Base::implicit_activation (adapter))
					POA_Base::implicit_activate (adapter, *this);
			}
		}
	} catch (...) {
		ServantProxyBase::_remove_ref ();
		throw;
	}
}

void ServantProxyObject::delete_servant (bool from_destructor)
{
	// Called in the servant synchronization context.
	assert (&Nirvana::Core::SyncContext::current () == sync_context_);
	assert (from_destructor || servant ());
	if (!servant ()) {
		assert (from_destructor);
		return;
	}

	if (ref_cnt_.load () > 0)
		return;

	if (reference_.load ()) { // Object has at least one active reference
		// Enter the POA context.
		// We don't need exception handling here because on_servant_destruct is noexcept.
		// So we don't use SYNC_BEGIN/SYNC_END and just create the sync frame.
		Nirvana::Core::Synchronized _sync_frame (*adapter_context_, nullptr);

		// Proxy reference can be added during the entering to new context
		if (ref_cnt_.load () > 0)
			return;

		// Now we are going to delete servant.
		// Clear all weak references to it.

		if (references_.empty ()) {
			// If the set is empty, object can have one reference stored in reference_.
			ReferenceLocal* ref = reference_.load ();
			if (ref)
				ref->on_servant_destruct ();
		} else {
			// If the set is not empty, it contains all the references include stored in reference_.
			for (ReferenceLocal* p : references_) {
				p->on_servant_destruct ();
			}

			// Clear map here, while we are in the POA sync context
			references_.clear ();
		}
	}

	PortableServer::ServantBase::_ptr_type srv = servant ();
	assert (from_destructor || srv);
	if (srv) {
		reset_servant ();
		if (!from_destructor)
			srv->_delete_object (); // Call servant destructor
	}
}

Boolean ServantProxyObject::non_existent ()
{
	return servant ()->_non_existent ();
}

Boolean ServantProxyObject::_is_equivalent (Object::_ptr_type other_object) const noexcept
{
	if (!other_object)
		return false;

	if (Base::_is_equivalent (other_object))
		return true;

	Reference* ref = ProxyManager::cast (other_object)->to_reference ();
	if (!ref || !(ref->flags () & Reference::LOCAL))
		return false;

	return static_cast <ReferenceLocal&> (*ref).get_active_servant_with_lock () == this;
}

ReferenceLocalRef ServantProxyObject::get_local_reference () const noexcept
{
	ReferenceLocalRef ref (reference_.lock ());
	reference_.unlock ();
	return ref;
}

ReferenceLocalRef ServantProxyObject::get_local_reference (const PortableServer::Core::POA_Base& adapter)
{
	// Called from the POA sync context
	assert (&SyncContext::current () == adapter_context_);
	if (references_.empty ()) {
		ReferenceLocalRef ref (reference_.load ());
		if (ref && &ref->adapter () == &adapter)
			return ref;
	} else {
		for (auto p : references_) {
			if (&p->adapter () == &adapter)
				return p;
		}
	}
	return nullptr;
}

ReferenceRef ServantProxyObject::marshal (StreamOut& out)
{
	ReferenceRef ref (get_local_reference ());
	if (!ref) {
		// Attempt to pass an unactivated (unregistered) value as an object reference.
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
	}
	return ref->marshal (out);
}

Policy::_ref_type ServantProxyObject::_get_policy (PolicyType policy_type)
{
	ReferenceRef ref (get_local_reference ());
	if (ref)
		return ref->_get_policy (policy_type);
	return Policy::_nil ();
}

DomainManagersList ServantProxyObject::_get_domain_managers ()
{
	return DomainManagersList (); // TODO: Implement
}

}
}
