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
#include "LocalAddress.h"
#include "StreamOutEncap.h"
#include "IIOP.h"
#include "ESIOP.h"
#include <CORBA/Servant_var.h>

using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

ReferenceLocal::ReferenceLocal (const PortableServer::Core::ObjectKey& key,
	const IDL::String& primary_iid, unsigned flags) :
	Reference (primary_iid, flags | LOCAL),
	object_key_ (key),
	root_ (PortableServer::Core::POA_Base::root ()),
	servant_ (nullptr)
{}

ReferenceLocal::ReferenceLocal (const PortableServer::Core::ObjectKey& key,
	PortableServer::Core::ServantBase& servant, unsigned flags) :
	Reference (servant.proxy (), flags | LOCAL),
	object_key_ (key),
	root_ (PortableServer::Core::POA_Base::root ()),
	servant_ (nullptr)
{}

ReferenceLocal::~ReferenceLocal ()
{}

void ReferenceLocal::_add_ref ()
{
	if (flags_ & LOCAL_WEAK) {
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
		if ((flags_ & LOCAL_WEAK) && servant) {
			servant->proxy ()._remove_ref ();
			servant = servant_.load ();
		}

		if (!servant || (flags_ & LOCAL_AUTO_DEACTIVATE)) {
			// Need GC
			SyncContext& adapter_context = local2proxy (Services::bind (Services::RootPOA))->sync_context ();
			if (&SyncContext::current () == &adapter_context) {
				// Do GC
				if (!servant)
					root_->remove_reference (object_key_);
				else if (flags_ & LOCAL_AUTO_DEACTIVATE) {
					PortableServer::Core::POA_Ref adapter = PortableServer::Core::POA_Root::find_child (object_key_.adapter_path (), false);
					if (adapter)
						adapter->deactivate_object (*this);
					else {
						ReferenceLocalRef ref (this);
						deactivate ();
					}
				}
			} else // Schedule GC
				GarbageCollector::schedule (*this, adapter_context);
		}
	}
}

void ReferenceLocal::activate (PortableServer::Core::ServantBase& servant, unsigned flags)
{
	// Caller must hold reference.
	assert (_refcount_value ());

	if (static_cast <PortableServer::Core::ServantBase*> (servant_.load ()))
		throw PortableServer::POA::ObjectAlreadyActive ();

	ProxyObject& proxy = servant.proxy ();
	check_primary_interface (proxy.primary_interface_id ());
	proxy.activate (*this);
	proxy._add_ref ();
	servant_ = &servant;
	flags_ = flags | LOCAL;
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

	if (flags_ & LOCAL_WEAK)
		flags_ &= ~(GARBAGE_COLLECTION | LOCAL_WEAK);

	if (p)
		p->deactivate (*this);
	return p;
}

void ReferenceLocal::on_destruct_implicit (PortableServer::Core::ServantBase& servant) NIRVANA_NOEXCEPT
{
	ServantPtr::Ptr ptr (&servant);
	if (servant_.cas (ptr, nullptr)) {
		flags_ &= LOCAL;

		PortableServer::Core::POA_Ref adapter = PortableServer::Core::POA_Root::find_child (object_key_.adapter_path (), false);
		if (adapter)
			adapter->implicit_deactivate (*this, servant.proxy ());

		// Toggle reference counter to invoke GC
		_add_ref ();
		_remove_ref ();
	}
}

Nirvana::Core::Ref <ProxyObject> ReferenceLocal::get_servant () const NIRVANA_NOEXCEPT
{
	Nirvana::Core::Ref <ProxyObject> ret;
	PortableServer::Core::ServantBase* servant = servant_.lock ();
	if (servant)
		ret = &servant->proxy ();
	servant_.unlock ();
	return ret;
}

void ReferenceLocal::marshal (StreamOut& out) const
{
	out.write_string_c (primary_interface_id ());
	ImplStatic <StreamOutEncap> encap;
	{
		IIOP::Version ver (1, 1);
		encap.write_c (alignof (IIOP::Version), sizeof (IIOP::Version), &ver);
	}
	encap.write_string_c (LocalAddress::singleton ().host ());
	UShort port = LocalAddress::singleton ().port ();
	encap.write_c (alignof (UShort), sizeof (UShort), &port);
	object_key_.marshal (encap);

	uint32_t ORB_type = ESIOP::ORB_TYPE;
	ESIOP::ProtDomainId domain_id = ESIOP::current_domain_id ();

	IOP::TaggedComponentSeq components;
#ifndef NIRVANA_SINGLE_DOMAIN
	components.reserve (3);
#else
	components.reserve (2);
#endif

	{
		ImplStatic <StreamOutEncap> encap;
		encap.write_c (4, 4, &ORB_type);
		components.emplace_back (IOP::TAG_ORB_TYPE, std::move (encap.data ()));
	}
#ifndef NIRVANA_SINGLE_DOMAIN
	{
		ImplStatic <StreamOutEncap> encap;
		encap.write_c (alignof (ESIOP::ProtDomainId), sizeof (ESIOP::ProtDomainId), &domain_id);
		components.emplace_back (ESIOP::TAG_DOMAIN_ADDRESS, std::move (encap.data ()));
	}
#endif
	{
		ImplStatic <StreamOutEncap> encap;
		Octet flags = (Octet)flags_;
		encap.write_c (1, 1, &flags);
		components.emplace_back (ESIOP::TAG_FLAGS, std::move (encap.data ()));
	}

	encap.write_tagged (components);

	IOP::TaggedProfileSeq addr {
		IOP::TaggedProfile (IOP::TAG_INTERNET_IOP, std::move (encap.data ()))
	};
	out.write_tagged (addr);
}

IORequest::_ref_type ReferenceLocal::create_request (OperationIndex op, unsigned flags)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags);

	Ref <ProxyObject> servant = get_servant ();
	if (servant)
		return servant->create_request (op, flags);

	check_create_request (op, flags);

	unsigned response_flags = flags & 3;
	if (flags & REQUEST_ASYNC)
		return make_pseudo <RequestLocalImpl <RequestLocalAsyncPOA> > (std::ref (*this), op,
			response_flags);
	else
		return make_pseudo <RequestLocalImpl <RequestLocalPOA> > (std::ref (*this), op,
			response_flags);
}

}
}
