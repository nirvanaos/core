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
#include "IDL/PortableServer.h"

namespace CORBA {
namespace Internal {
namespace Core {

using namespace ::Nirvana::Core;

class ProxyObject::Deactivator :
	public CoreObject,
	public Runnable
{
public:
	Deactivator (ProxyObject& proxy) :
		proxy_ (proxy)
	{}

	~Deactivator ()
	{}

	void run ()
	{
		proxy_.implicit_deactivate ();
	}

private:
	ProxyObject& proxy_;
};

// Called in the servant synchronization context.
// Note that sync context may be out of synchronization domain
// for the stateless objects.
void ProxyObject::add_ref_1 ()
{
	Base::add_ref_1 ();
	if (
		!change_state (DEACTIVATION_SCHEDULED, DEACTIVATION_CANCELLED)
	&&
		change_state (INACTIVE, ACTIVATION)
	) {
		try {
			PortableServer::POA::_ref_type poa = servant_->_default_POA ();
			// TODO: Query poa for the implicit activation policy
			// While assume that implicit activation is on
			implicit_activation_ = true;

			assert (&sync_context () == &SyncContext::current ());

			if (sync_context ().is_free_sync_context ()) {
				// If target in a free sync context, implicit_activated_id_ must be
				// allocated in the g_shared_mem_context.
				// Otherwise it's data will be lost on the pop current memory context.
				ExecDomain* ed = Thread::current ().exec_domain ();
				ed->mem_context_push (&g_shared_mem_context);
				implicit_activated_id_ = poa->activate_object (servant_);
				ed->mem_context_pop ();
				assert (g_shared_mem_context->heap ().check_owner (
					implicit_activated_id_.data (), implicit_activated_id_.capacity ()));
			} else {
				implicit_activated_id_ = poa->activate_object (servant_);
				assert (sync_context ().sync_domain ()->mem_context ().heap ().check_owner (
					implicit_activated_id_.data (), implicit_activated_id_.capacity ()));
				assert (&MemContext::current () == &sync_context ().sync_domain ()->mem_context ());
			}
			activation_state_ = ACTIVE;
		} catch (...) {
			activation_state_ = INACTIVE;
			throw;
		}
	}
}

::Nirvana::Core::RefCounter::IntegralType ProxyObject::_remove_ref () NIRVANA_NOEXCEPT
{
	::Nirvana::Core::RefCounter::IntegralType cnt = Base::_remove_ref ();
	if (implicit_activation_ && 1 == cnt) {
		// Launch deactivator
		if (
			!change_state (DEACTIVATION_CANCELLED, DEACTIVATION_SCHEDULED)
		&&
			change_state (ACTIVE, DEACTIVATION_SCHEDULED)
		) {
			run_garbage_collector <Deactivator> (std::ref (*this));
		}
	}
	return cnt;
}

// Called from Deactivator in the servant synchronization context.
void ProxyObject::implicit_deactivate ()
{
	assert (&sync_context () == &SyncContext::current ());

	// Object may be re-activated later. So we clear implicit_activated_id_ here.
	PortableServer::ObjectId tmp = std::move (implicit_activated_id_);
	if (change_state (DEACTIVATION_SCHEDULED, INACTIVE)) {
		servant_->_default_POA ()->deactivate_object (tmp);
	} else {
		// Restore implicit_activated_id_
		implicit_activated_id_ = std::move (tmp);
		assert (DEACTIVATION_CANCELLED == activation_state_);
		if (!change_state (DEACTIVATION_CANCELLED, ACTIVE))
			::Nirvana::throw_BAD_INV_ORDER ();
	}
	if (!tmp.empty () && sync_context ().is_free_sync_context ()) {
		// If target in a free sync context, implicit_activated_id_ must be
		// allocated in the g_shared_mem_context.
		assert (g_shared_mem_context->heap ().check_owner (
			tmp.data (), tmp.capacity ()));
		ExecDomain* ed = Thread::current ().exec_domain ();
		ed->mem_context_push (&g_shared_mem_context);
		tmp.clear ();
		tmp.shrink_to_fit ();
		ed->mem_context_pop ();
	} else {
		assert (sync_context ().sync_domain ()->mem_context ().heap ().check_owner (
			tmp.data (), tmp.capacity ()));
		assert (&MemContext::current () == &sync_context ().sync_domain ()->mem_context ());
	}
}

void ProxyObject::non_existent_request (ProxyObject* _servant, IORequest::_ptr_type _rq)
{
	Boolean _ret = _servant->servant_->_non_existent ();
	Type <Boolean>::marshal_out (_ret, _rq);
}

const Operation ProxyObject::object_ops_ [3] = {
	{ op_get_interface_, {0, 0}, {0, 0}, Type <InterfaceDef>::type_code, nullptr },
	{ op_is_a_, {&is_a_param_, 1}, {0, 0}, Type <Boolean>::type_code, nullptr },
	{ op_non_existent_, {0, 0}, {0, 0}, Type <Boolean>::type_code, ObjProcWrapper <ProxyObject, non_existent_request> }
};

}
}
}
