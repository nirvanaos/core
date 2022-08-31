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
#include "../Runnable.h"
#include "POA_Root.h"

using namespace Nirvana::Core;
using namespace std;

namespace CORBA {

using namespace Internal;

namespace Core {

std::atomic <int> ProxyObject::next_timestamp_;

Object::_ptr_type servant2object (PortableServer::Servant servant) NIRVANA_NOEXCEPT
{
	if (servant) {
		PortableServer::Servant ps = servant->_core_servant ();
		return static_cast <PortableServer::Core::ServantBase*> (
			static_cast <CORBA::Internal::Bridge <PortableServer::ServantBase>*> (&ps))->get_proxy ();
	} else
		return nullptr;
}

PortableServer::ServantBase::_ref_type object2servant (CORBA::Object::_ptr_type obj)
{
	if (obj) {
		const CORBA::Core::ProxyObject* proxy = object2proxy (obj);

		if (&proxy->sync_context () != &Nirvana::Core::SyncContext::current ())
			throw CORBA::OBJ_ADAPTER ();
		return proxy->servant ();
	}
	return nullptr;
}

class NIRVANA_NOVTABLE ProxyObject::Deactivator :
	public UserObject,
	public Runnable
{
public:
	void run ()
	{
		proxy_.implicit_deactivate ();
	}

protected:
	Deactivator (ProxyObject& proxy) :
		proxy_ (proxy)
	{}

	~Deactivator ()
	{}

private:
	ProxyObject& proxy_;
};

void ProxyObject::activate (ActivationKeyRef&& key) NIRVANA_NOEXCEPT
{
	activation_key_ = std::move (key);
	activation_key_memory_ = &MemContext::current ();
	activation_state_ = ActivationState::ACTIVE;
}

void ProxyObject::deactivate () NIRVANA_NOEXCEPT
{
	// Called from POA sync domain, so memory context must be the same.
	activation_key_.reset ();
	activation_key_memory_.reset ();
	activation_state_ = ActivationState::INACTIVE;
}

// Called in the servant synchronization context.
// Note that sync context may be out of synchronization domain
// for the stateless objects.
void ProxyObject::add_ref_1 ()
{
	Base::add_ref_1 ();

	if (!change_state (DEACTIVATION_SCHEDULED, DEACTIVATION_CANCELLED)) {
		PortableServer::POA::_ref_type poa = servant ()->_default_POA ();
		// Query poa for the implicit activation policy
		if (PortableServer::Core::POA_Base::implicit_activation (poa) && change_state (INACTIVE, ACTIVATION)) {
			implicit_POA_ = move (poa);
			try {
				implicit_POA_->activate_object (servant ());
			} catch (...) {
				implicit_POA_ = nullptr;
				if (change_state (ACTIVATION, INACTIVE))
					throw;
			}
		}
	}
}

void ProxyObject::_remove_ref () NIRVANA_NOEXCEPT
{
	ProxyObject::RefCnt::IntegralType cnt = Base::_remove_ref_proxy ();
	if (1 == cnt && implicit_POA_) {
		// Launch deactivator
		if (
			!change_state (DEACTIVATION_CANCELLED, DEACTIVATION_SCHEDULED)
		&&
			change_state (ACTIVE, DEACTIVATION_SCHEDULED)
		) {
			run_garbage_collector <Deactivator> (std::ref (*this),
				local2proxy (implicit_POA_)->sync_context ());
		}
	}
}

// Called from Deactivator in the POA synchronization context.
void ProxyObject::implicit_deactivate ()
{
	// Object may be re-activated later. So we clear implicit_activated_id_ here.
	if (change_state (DEACTIVATION_SCHEDULED, INACTIVE)) {
		ActivationKeyRef key = activation_key_.get ();
		assert (implicit_POA_ && key);
		PortableServer::POA::_ref_type poa (move (implicit_POA_));
		const ProxyLocal* proxy = local2proxy (poa);
		assert (&proxy->sync_context () == &SyncContext::current ());
		PortableServer::Core::POA_Base* poa_impl = PortableServer::Core::POA_Base::get_implementation (proxy);
		assert (poa_impl);
		poa_impl->deactivate_object (key->object_id);
	} else {
		// Restore implicit_activated_id_
		assert (DEACTIVATION_CANCELLED == activation_state_);
		if (!change_state (DEACTIVATION_CANCELLED, ACTIVE))
			::Nirvana::throw_BAD_INV_ORDER ();
	}
}

Boolean ProxyObject::non_existent ()
{
	return servant ()->_non_existent ();
}

}
}
