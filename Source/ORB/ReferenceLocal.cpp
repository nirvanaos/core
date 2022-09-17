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
#include "ReferenceLocal.h"
#include "RequestLocalPOA.h"
#include "POA_Root.h"
#include <CORBA/Servant_var.h>

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

class ReferenceLocal::GC :
	public Runnable,
	public SharedObject
{
public:
	GC (ReferenceLocal& ref) :
		ref_ (&ref)
	{}

	virtual void run () override
	{
		ref_ = nullptr;
	}

private:
	ReferenceLocalRef ref_;
};

ReferenceLocal::ReferenceLocal (PortableServer::Core::ObjectKey&& key,
	const IDL::String& primary_iid, bool garbage_collection) :
	PortableServer::Core::ObjectKey (std::move (key)),
	Reference (primary_iid, garbage_collection),
	root_ (PortableServer::Core::POA_Base::root ()),
	servant_ (nullptr)
{}

ReferenceLocal::ReferenceLocal (PortableServer::Core::ObjectKey&& key,
	PortableServer::Core::ServantBase& servant, bool garbage_collection) :
	PortableServer::Core::ObjectKey (std::move (key)),
	Reference (servant.proxy (), garbage_collection),
	root_ (PortableServer::Core::POA_Base::root ()),
	servant_ (&servant)
{
	servant.proxy ()._add_ref ();
}

ReferenceLocal* ReferenceLocal::local_reference ()
{
	return this;
}

void ReferenceLocal::_add_ref ()
{
	if (garbage_collection_) {
		if (1 == ref_cnt_.increment_seq ()) {
			PortableServer::Core::ServantBase* servant = servant_.lock ();
			if (servant)
				servant->proxy ()._add_ref ();
			servant_.unlock ();
		}
	} else
		ref_cnt_.increment ();
}

void ReferenceLocal::_remove_ref () NIRVANA_NOEXCEPT
{
	if (0 == ref_cnt_.decrement_seq ()) {
		PortableServer::Core::ServantBase* servant = servant_.load ();
		if (garbage_collection_ && servant) {
			servant->proxy ()._remove_ref ();
			servant = servant_.load ();
		}

		if (!servant && !garbage_collection_) { // Can be removed
			SyncContext& adapter_context = local2proxy (Services::bind (Services::RootPOA))->sync_context ();
			if (&SyncContext::current () == &adapter_context)
				root_->remove_reference (*this);
			else {
				try {
					Nirvana::DeadlineTime deadline =
						Nirvana::Core::PROXY_GC_DEADLINE == Nirvana::INFINITE_DEADLINE ?
						Nirvana::INFINITE_DEADLINE : Nirvana::Core::Chrono::make_deadline (
							Nirvana::Core::PROXY_GC_DEADLINE);

					ExecDomain::async_call (deadline,
						CoreRef <Runnable>::create <ImplDynamic <GC> > (std::ref (*this)),
						adapter_context);
				} catch (...) {
					// TODO: Log
				}
			}
		}
	}
}

void ReferenceLocal::activate (PortableServer::Core::ServantBase& servant)
{
	if (static_cast <PortableServer::Core::ServantBase*> (servant_.load ()))
		throw PortableServer::POA::ObjectAlreadyActive ();

	ProxyObject& proxy = servant.proxy ();
	ProxyManager::operator = (proxy);
	proxy.activate (*this);
	proxy._add_ref ();
	servant_ = &servant;
}

servant_reference <ProxyObject> ReferenceLocal::deactivate () NIRVANA_NOEXCEPT
{
	// Caller must hold reference.
	assert (_refcount_value ());

	PortableServer::Core::ServantBase* servant = servant_.exchange (nullptr);
	// ProxyObject is always add-refed there so we use Servant_var.
	PortableServer::Servant_var <ProxyObject> p;
	if (servant)
		p = &servant->proxy (); // Servant_var do not call _add_ref

	// Explicitly deactivated objects do not involved in GC
	garbage_collection_ = false;

	if (p)
		p->deactivate (*this);
	return p;
}

void ReferenceLocal::on_destruct_implicit (PortableServer::Core::ServantBase& servant) NIRVANA_NOEXCEPT
{
	ServantPtr::Ptr ptr (&servant);
	if (servant_.cas (ptr, nullptr)) {
		garbage_collection_ = false;
		
		PortableServer::Core::POA_Ref adapter = PortableServer::Core::POA_Root::find_child (adapter_path ());
		if (adapter)
			adapter->implicit_deactivate (*this, servant.proxy ());

		// Toggle reference counter to invoke GC
		_add_ref ();
		_remove_ref ();
	}
}

Nirvana::Core::CoreRef <ProxyObject> ReferenceLocal::get_servant () const NIRVANA_NOEXCEPT
{
	Nirvana::Core::CoreRef <ProxyObject> ret;
	PortableServer::Core::ServantBase* servant = servant_.lock ();
	if (servant)
		ret = &servant->proxy ();
	servant_.unlock ();
	return ret;
}

void ReferenceLocal::marshal (StreamOut& out) const
{
	out.write_string (static_cast <const IDL::String&> (primary_interface_id ()));
	throw NO_IMPLEMENT (); // TODO: Implement.
	PortableServer::Core::ObjectKey::marshal (out);
}

IORequest::_ref_type ReferenceLocal::create_request (OperationIndex op, UShort flags)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags);

	CoreRef <ProxyObject> servant = get_servant ();
	if (servant)
		return servant->create_request (op, flags);

	check_create_request (op, flags);

	UShort response_flags = flags & 3;
	if (flags & REQUEST_ASYNC)
		return make_pseudo <RequestLocalImpl <RequestLocalAsyncPOA> > (std::ref (*this), op,
			response_flags);
	else
		return make_pseudo <RequestLocalImpl <RequestLocalPOA> > (std::ref (*this), op,
			response_flags);
}

}
}
