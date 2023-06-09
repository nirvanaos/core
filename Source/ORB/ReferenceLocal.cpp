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
#include <CORBA/IIOP.h>
#include "ESIOP.h"
#include <CORBA/Servant_var.h>

using namespace Nirvana::Core;
using namespace PortableServer::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

ReferenceLocal::ReferenceLocal (const IOP::ObjectKey& object_key, PortableServer::Core::ObjectKey&& core_key,
	const IDL::String& primary_iid, unsigned flags, PolicyMapShared* policies) :
	Reference (primary_iid, flags | LOCAL),
	core_key_ (std::move (core_key)),
	object_key_ (object_key),
	adapter_context_ (&local2proxy (POA_Root::get_root ())->sync_context ()),
	root_ (PortableServer::Core::POA_Base::root ()),
	servant_ (nullptr)
{
	policies_ = policies;
}

ReferenceLocal::ReferenceLocal (const IOP::ObjectKey& object_key, PortableServer::Core::ObjectKey&& core_key,
	ServantProxyObject& proxy, unsigned flags, PolicyMapShared* policies) :
	Reference (proxy, flags | LOCAL),
	core_key_ (std::move (core_key)),
	object_key_ (object_key),
	adapter_context_ (&local2proxy (POA_Root::get_root ())->sync_context ()),
	root_ (PortableServer::Core::POA_Base::root ()),
	servant_ (nullptr)
{
	policies_ = policies;
}

ReferenceLocal::~ReferenceLocal ()
{}

void ReferenceLocal::_add_ref ()
{
	if (flags_ & GARBAGE_COLLECTION) {
		if (1 == ref_cnt_.increment_seq ()) {
			ServantProxyObject* proxy = servant_.lock ();
			if (proxy)
				proxy->_add_ref ();
			servant_.unlock ();
		}
	} else
		ref_cnt_.increment ();
}

void ReferenceLocal::_remove_ref () NIRVANA_NOEXCEPT
{
	if (0 == ref_cnt_.decrement_seq ()) {
		if (flags_ & GARBAGE_COLLECTION) {
			ServantProxyObject* proxy = servant_.load ();
			if (proxy) {
				proxy->_remove_ref ();
				return;
			}
		} else if (servant_.load ()) // Servant is still active
			return;

		// Need GC
		if (&SyncContext::current () == adapter_context_) {
			// Do GC
			root_->remove_reference (object_key_);
		} else {
			// Schedule GC
			GarbageCollector::schedule (*this, *adapter_context_);
		}
	}
}

void ReferenceLocal::activate (ServantProxyObject& proxy)
{
	// Caller must hold both references.
	assert (_refcount_value ());
	assert (proxy._refcount_value ());

	if (servant_.load ())
		throw PortableServer::POA::ObjectAlreadyActive ();

	check_primary_interface (proxy.primary_interface_id ());
	proxy.activate (*this);
	proxy._add_ref ();
	servant_ = &proxy;
}

servant_reference <ServantProxyObject> ReferenceLocal::deactivate () NIRVANA_NOEXCEPT
{
	// Caller must hold reference.
	assert (_refcount_value ());

	// ServantProxyObject is always add-refed there so we use Servant_var.
	PortableServer::Servant_var <ServantProxyObject> proxy (servant_.exchange (nullptr));

	if (proxy)
		proxy->deactivate (*this);
	return proxy;
}

void ReferenceLocal::on_servant_destruct () NIRVANA_NOEXCEPT
{
	// Called on the active weak reference servant destruction.
	assert (&SyncContext::current () == adapter_context_);
	ServantProxyObject* proxy = servant_.exchange (nullptr);
	assert (proxy);
	PortableServer::Core::POA_Ref adapter = PortableServer::Core::POA_Root::find_child (core_key_.adapter_path (), false);
	if (adapter)
		adapter->implicit_deactivate (*this, *proxy);

	// Toggle reference counter to invoke GC
	_add_ref ();
	_remove_ref ();
}

Ref <ServantProxyObject> ReferenceLocal::get_active_servant () const NIRVANA_NOEXCEPT
{
	// This method is always called from the POA sync context, so we need not lock the pointer.
	assert (&SyncContext::current () == adapter_context_);
	return Ref <ServantProxyObject> (servant_.load ());
}

void ReferenceLocal::marshal (const ProxyManager& proxy, const Octet* obj_key, size_t obj_key_size,
	const PolicyMap* policies, StreamOut& out)
{
	out.write_string_c (proxy.primary_interface_id ());
	ImplStatic <StreamOutEncap> encap;
	{
		IIOP::Version ver (1, 1);
		encap.write_c (alignof (IIOP::Version), sizeof (IIOP::Version), &ver);
	}
	encap.write_string_c (LocalAddress::singleton ().host ());
	UShort port = LocalAddress::singleton ().port ();
	encap.write_c (alignof (UShort), sizeof (UShort), &port);
	
	size_t zero = 0;
	encap.write_seq (1, 1, obj_key_size, const_cast <Octet*> (obj_key), zero);

	uint32_t ORB_type = ESIOP::ORB_TYPE;
	ESIOP::ProtDomainId domain_id = ESIOP::current_domain_id ();

	size_t component_cnt = Nirvana::Core::SINGLE_DOMAIN ? 2 : 3;

	if (policies) {
		if (!policies->empty ())
			++component_cnt;
		else
			policies = nullptr;
	}

	IOP::TaggedComponentSeq components;
	components.reserve (component_cnt);

	{ // IOP::TAG_ORB_TYPE
		ImplStatic <StreamOutEncap> encap;
		encap.write_c (4, 4, &ORB_type);
		components.emplace_back (IOP::TAG_ORB_TYPE, std::move (encap.data ()));
	}

	{ // IOP::TAG_FT_HEARTBEAT_ENABLED
		ImplStatic <StreamOutEncap> encap;
		uint8_t enabled = 1;
		encap.write_c (1, 1, &enabled);
		components.emplace_back (IOP::TAG_FT_HEARTBEAT_ENABLED, std::move (encap.data ()));
	}
	
	if (!Nirvana::Core::SINGLE_DOMAIN) {
		ImplStatic <StreamOutEncap> encap;
		encap.write_c (alignof (ESIOP::ProtDomainId), sizeof (ESIOP::ProtDomainId), &domain_id);
		components.emplace_back (ESIOP::TAG_DOMAIN_ADDRESS, std::move (encap.data ()));
	}

	if (policies) {
		ImplStatic <StreamOutEncap> encap;
		encap.write_size (policies->size ());
		policies->write (encap);
		components.emplace_back (IOP::TAG_POLICIES, std::move (encap.data ()));
	}

	encap.write_tagged (components);

	IOP::TaggedProfileSeq addr {
		IOP::TaggedProfile (IOP::TAG_INTERNET_IOP, std::move (encap.data ()))
	};
	out.write_tagged (addr);
}

ReferenceRef ReferenceLocal::marshal (StreamOut& out)
{
	marshal (*this, object_key_.data (), object_key_.size (), policies_, out);
	return this;
}

ReferenceLocalRef ReferenceLocal::get_local_reference (const PortableServer::Core::POA_Base& adapter)
{
	// Called from the POA sync context
	assert (&SyncContext::current () == adapter_context_);
	if (adapter.check_path (core_key_.adapter_path ()))
		return this;
	else
		return nullptr;
}

IORequest::_ref_type ReferenceLocal::create_request (OperationIndex op, unsigned flags, Internal::RequestCallback::_ptr_type callback)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags, callback);

	// If servant is active, create direct request for performance.
	// We can't use get_active_servant here because the arbitrary sync context.
	// So we lock the proxy pointer here.
	Ref <ServantProxyObject> proxy (servant_.lock ());
	servant_.unlock ();
	if (proxy)
		return proxy->create_request (op, flags, callback);

	// Create POA request.
	check_create_request (op, flags);

	if (callback) {
		if (!(flags & Internal::IORequest::RESPONSE_EXPECTED))
			throw BAD_PARAM ();
		return make_pseudo <RequestLocalImpl <RequestLocalAsyncPOA> > (callback, std::ref (*this), op, flags);
	} else if (flags & Internal::IORequest::RESPONSE_EXPECTED) {
		return make_pseudo <RequestLocalImpl <RequestLocalPOA> > (std::ref (*this), op, flags);
	} else {
		return make_pseudo <RequestLocalImpl <RequestLocalOnewayPOA> > (std::ref (*this), op, flags);
	}
}

DomainManagersList ReferenceLocal::_get_domain_managers ()
{
	throw NO_IMPLEMENT ();
}

}
}
