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
#include "POA_Root.h"

using namespace Nirvana::Core;
using namespace PortableServer::Core;

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
		const CORBA::Core::ServantProxyObject* proxy = object2proxy (obj);

		if (&proxy->sync_context () != &Nirvana::Core::SyncContext::current ())
			throw CORBA::OBJ_ADAPTER ();
		return proxy->servant ();
	}
	return nullptr;
}

ServantProxyObject::ServantProxyObject (PortableServer::Servant user_servant) :
	ServantProxyBase (user_servant),
	adapter_context_ (&local2proxy (POA_Root::get_root ())->sync_context ())
{}

ServantProxyObject::~ServantProxyObject ()
{
	try {
		if (references_.empty ()) {
			// If the set is empty, object can have one reference stored in reference_.
			ReferenceLocal* ref = reference_.load ();
			if (ref) {
				// We don't need exception handling here because on_destruct_implicit is noexcept.
				// So we don't use SYNC_BEGIN/SYNC_END and just create the sync frame.
				Nirvana::Core::Synchronized _sync_frame (*adapter_context_, nullptr);
				ref->on_servant_destruct ();
			}
		} else {
			// If the set is not empty, it contains all the references include stored in reference_.
			// We don't need exception handling here because on_destruct_implicit is noexcept.
			// So we don't use SYNC_BEGIN/SYNC_END and just create the sync frame.
			Nirvana::Core::Synchronized _sync_frame (*adapter_context_, nullptr);
			for (void* p : references_) {
				reinterpret_cast <ReferenceLocal*> (p)->on_servant_destruct ();
			}
		}
	} catch (...) {
		assert (false);
	}
}

void ServantProxyObject::_add_ref ()
{
	RefCntProxy::IntegralType cnt = ref_cnt_.increment_seq ();
	if (1 == cnt && servant_) {
		add_ref_servant ();
		if (!reference_.load ()) {
			PortableServer::POA::_ref_type adapter = servant ()->_default_POA ();
			if (adapter && POA_Base::implicit_activation (adapter) != POA_Base::NO_IMPLICIT_ACTIVATION)
				POA_Base::implicit_activate (adapter, *this);
		}
	}
}

Boolean ServantProxyObject::non_existent ()
{
	return servant ()->_non_existent ();
}

ReferenceLocalRef ServantProxyObject::get_reference_local () const NIRVANA_NOEXCEPT
{
	ReferenceLocalRef ref (reference_.lock ());
	reference_.unlock ();
	return ref;
}

ReferenceRef ServantProxyObject::get_reference ()
{
	ReferenceRef ref (get_reference_local ());
	if (!ref) // Attempt to pass an unactivated (unregistered) value as an object reference.
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
	return ref;
}

void ServantProxyObject::marshal (StreamOut& out) const
{
	ReferenceLocalRef ref = get_reference_local ();
	if (!ref) // Attempt to pass an unactivated (unregistered) value as an object reference.
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (1));
	ref->marshal (out);
}

Policy::_ref_type ServantProxyObject::_get_policy (PolicyType policy_type)
{
	return get_reference ()->_get_policy (policy_type);
}

DomainManagersList ServantProxyObject::_get_domain_managers ()
{
	return get_reference ()->_get_domain_managers ();
}

}
}
