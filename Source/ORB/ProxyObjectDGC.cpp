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
#include "ProxyObjectDGC.h"
#include "POA_Base.h"
#include "StreamOutEncap.h"

using namespace Nirvana::Core;
using namespace PortableServer;
using namespace PortableServer::Core;

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE ProxyObjectDGC::Deactivator :
	public UserObject,
	public Runnable
{
public:
	void run ()
	{
		proxy_.implicit_deactivate ();
	}

protected:
	Deactivator (ProxyObjectDGC& proxy) :
		proxy_ (proxy)
	{}

	~Deactivator ()
	{}

private:
	ProxyObjectDGC& proxy_;
};

ProxyObjectDGC::ProxyObjectDGC (PortableServer::Servant servant, POA::_ptr_type adapter) :
	Base (servant),
	adapter_ (adapter),
	activation_state_ (ActivationState::INACTIVE)
{}

ProxyObjectDGC::ProxyObjectDGC (const ProxyObject& src, POA::_ptr_type adapter) :
	Base (src),
	adapter_ (adapter),
	activation_state_ (ActivationState::INACTIVE)
{}

void ProxyObjectDGC::activate (ObjectKey&& key)
{
	Base::activate (std::move (key));
	activation_state_ = ActivationState::ACTIVE;
}

void ProxyObjectDGC::deactivate () NIRVANA_NOEXCEPT
{
	Base::deactivate ();
	activation_state_ = ActivationState::INACTIVE;
}

void ProxyObjectDGC::_remove_ref () NIRVANA_NOEXCEPT
{
	ProxyObject::RefCnt::IntegralType cnt = Base::remove_ref_proxy ();
	if (1 == cnt) {
		// Launch deactivator
		if (
			!change_state (DEACTIVATION_CANCELLED, DEACTIVATION_SCHEDULED)
			&&
			change_state (ACTIVE, DEACTIVATION_SCHEDULED)
			) {
			run_garbage_collector <Deactivator> (std::ref (*this), local2proxy (adapter_)->sync_context ());
		}
	}
}

// Called from Deactivator in the POA synchronization context.
void ProxyObjectDGC::implicit_deactivate ()
{
	// Object may be re-activated later. So we clear implicit_activated_id_ here.
	if (change_state (DEACTIVATION_SCHEDULED, INACTIVE)) {
		PortableServer::Core::ObjectKeyRef key = object_key_.get ();
		assert (key);
		const ProxyLocal* proxy = local2proxy (adapter_);
		assert (&proxy->sync_context () == &SyncContext::current ());
		PortableServer::Core::POA_Base* poa_impl = PortableServer::Core::POA_Base::get_implementation (proxy);
		assert (poa_impl);
		poa_impl->deactivate_object (key->key ().object_id ());
	} else {
		// Restore implicit_activated_id_
		assert (DEACTIVATION_CANCELLED == activation_state_);
		if (!change_state (DEACTIVATION_CANCELLED, ACTIVE))
			::Nirvana::throw_BAD_INV_ORDER ();
	}
}

}
}
