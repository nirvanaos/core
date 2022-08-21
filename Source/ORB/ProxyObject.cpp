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
#include "ProxyObject.h"
#include "../Runnable.h"
#include "POA_Root.h"

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

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

// Called in the servant synchronization context.
// Note that sync context may be out of synchronization domain
// for the stateless objects.
void ProxyObject::add_ref_1 ()
{
	Base::add_ref_1 ();

	if (!change_state (DEACTIVATION_SCHEDULED, DEACTIVATION_CANCELLED)) {
		PortableServer::POA::_ref_type poa = servant ()->_default_POA ();
		// Query poa for the implicit activation policy
		bool implicit_activation = PortableServer::Core::POA_Base::implicit_activation (poa);
		if (implicit_activation && change_state (INACTIVE, ACTIVATION)) {
			try {
				poa->activate_object (servant ());
			} catch (...) {
				if (change_state (ACTIVATION, INACTIVE))
					throw;
			}
		}
	}
}

ProxyObject::RefCnt::IntegralType ProxyObject::_remove_ref_proxy () NIRVANA_NOEXCEPT
{
	ProxyObject::RefCnt::IntegralType cnt = Base::_remove_ref_proxy ();
	if (implicit_activation_ && 1 == cnt) {
		// Launch deactivator
		if (
			!change_state (DEACTIVATION_CANCELLED, DEACTIVATION_SCHEDULED)
		&&
			change_state (ACTIVE, DEACTIVATION_SCHEDULED)
		) {
			assert (activated_POA_ && activated_id_);
			run_garbage_collector <Deactivator> (std::ref (*this),
				PortableServer::Core::POA_Base::get_proxy (activated_POA_)->sync_context ());
		}
	}
	return cnt;
}

// Called from Deactivator in the POA synchronization context.
void ProxyObject::implicit_deactivate ()
{
	// Object may be re-activated later. So we clear implicit_activated_id_ here.
	if (change_state (DEACTIVATION_SCHEDULED, INACTIVE)) {
		assert (activated_POA_ && activated_id_);
		const Core::LocalObject* proxy = PortableServer::Core::POA_Base::get_proxy (activated_POA_);
		assert (&proxy->sync_context () == &SyncContext::current ());
		PortableServer::Core::POA_Base* poa_impl = PortableServer::Core::POA_Base::get_implementation (proxy);
		assert (poa_impl);
		poa_impl->deactivate_object (*activated_id_);
	} else {
		// Restore implicit_activated_id_
		assert (DEACTIVATION_CANCELLED == activation_state_);
		if (!change_state (DEACTIVATION_CANCELLED, ACTIVE))
			::Nirvana::throw_BAD_INV_ORDER ();
	}
}

void ProxyObject::non_existent_request (ProxyObject* servant, IORequest::_ptr_type _rq)
{
	Boolean _ret = servant->servant ()->_non_existent ();
	Type <Boolean>::marshal_out (_ret, _rq);
}

const OperationsDII ProxyObject::object_ops_ = {
	{ op_get_interface_, {0, 0}, {0, 0}, Type <InterfaceDef>::type_code, nullptr },
	{ op_is_a_, {&is_a_param_, 1}, {0, 0}, Type <Boolean>::type_code, nullptr },
	{ op_non_existent_, {0, 0}, {0, 0}, Type <Boolean>::type_code, ObjProcWrapper <ProxyObject, non_existent_request> },
	{ op_repository_id_, {0, 0}, {0, 0}, Type <String>::type_code, nullptr }
};

}
}
