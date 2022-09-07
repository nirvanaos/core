/// \file
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
#ifndef NIRVANA_ORB_CORE_POA_BASE_H_
#define NIRVANA_ORB_CORE_POA_BASE_H_
#pragma once

#include <CORBA/Server.h>
#include "ProxyObject.h"
#include "ProxyLocal.h"
#include "MapUnorderedStable.h"
#include "HashOctetSeq.h"
#include "PortableServer_policies.h"
#include "RequestInPOA.h"
#include "../TLS.h"
#include "../Event.h"

namespace CORBA {
namespace Core {
class RequestInPOA;
}
}

namespace PortableServer {
namespace Core {

class POAManager;

struct Context
{
	POA::_ref_type adapter;
	const ObjectId& object_id;
	CORBA::Object::_ptr_type object;

	Context (POA::_ref_type&& poa, const ObjectId& oid, CORBA::Core::ProxyObject& proxy) :
		adapter (std::move (poa)),
		object_id (oid),
		object (proxy.get_proxy ())
	{}
};

class RequestRef : public Nirvana::Core::CoreRef <CORBA::Core::RequestInPOA>
{
	typedef Nirvana::Core::CoreRef <CORBA::Core::RequestInPOA> Base;
public:
	RequestRef (CORBA::Core::RequestInPOA& rq) :
		Base (&rq),
		memory_ (&Nirvana::Core::MemContext::current ())
	{}

	RequestRef (Nirvana::Core::CoreRef <CORBA::Core::RequestInPOA>&& rq) :
		Base (std::move (rq)),
		memory_ (&Nirvana::Core::MemContext::current ())
	{}

	RequestRef (const RequestRef&) = default;
	RequestRef (RequestRef&&) = default;

	~RequestRef ()
	{
		reset ();
	}

	RequestRef& operator = (const RequestRef&) = default;
	RequestRef& operator = (RequestRef&&) = default;

	void reset () NIRVANA_NOEXCEPT;

	Nirvana::Core::MemContext* memory () const NIRVANA_NOEXCEPT
	{
		return memory_;
	}

private:
	Nirvana::Core::CoreRef <Nirvana::Core::MemContext> memory_;
};

// POA implementation always operates the reference to Object interface (proxy),
// not to ServantBase.
// Release of the ServantBase reference must be performed in the ServantBase
// synchronization context, so the reference counting of the ServantBase is
// responsibility of the proxy.
// The POA never communicates with user ServantBase directly.

class NIRVANA_NOVTABLE POA_Base :
	public CORBA::servant_traits <POA>::Servant <POA_Base>
{
public:
	// POA creation and destruction

	POA::_ref_type create_POA (const IDL::String& adapter_name,
		PortableServer::POAManager::_ptr_type a_POAManager, const CORBA::PolicyList& policies)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	POA::_ref_type find_POA (const IDL::String& adapter_name, bool activate_it)
	{
		return find_child (adapter_name, activate_it)._this ();
	}

	POA_Base& find_child (const IDL::String& adapter_name, bool activate_it);

	static void check_wait_completion ();

	void destroy (bool etherealize_objects, bool wait_for_completion)
	{
		if (wait_for_completion)
			check_wait_completion ();

		if (!destroyed_) {
			if (parent_) {
				parent_->children_.erase (*name_);
				parent_ = nullptr;
			}

			destroy (etherealize_objects);
		}

		if (wait_for_completion)
			destroy_completed_.wait ();
	}

	virtual void destroy (bool etherealize_objects) NIRVANA_NOEXCEPT;

	bool is_destroyed () const NIRVANA_NOEXCEPT
	{
		return destroyed_;
	}

	virtual void etherealize_objects () {}

	// Factories for Policy objects

	static ThreadPolicy::_ref_type create_thread_policy (ThreadPolicyValue value)
	{
		return CORBA::Core::PolicyImpl <THREAD_POLICY_ID>::create (value);
	}

	static LifespanPolicy::_ref_type create_lifespan_policy (LifespanPolicyValue value)
	{
		return CORBA::Core::PolicyImpl <LIFESPAN_POLICY_ID>::create (value);
	}

	static IdUniquenessPolicy::_ref_type create_id_uniqueness_policy (IdUniquenessPolicyValue value)
	{
		return CORBA::Core::PolicyImpl <ID_UNIQUENESS_POLICY_ID>::create (value);
	}

	static IdAssignmentPolicy::_ref_type  create_id_assignment_policy (IdAssignmentPolicyValue value)
	{
		return CORBA::Core::PolicyImpl <ID_ASSIGNMENT_POLICY_ID>::create (value);
	}

	static ImplicitActivationPolicy::_ref_type create_implicit_activation_policy (ImplicitActivationPolicyValue value)
	{
		return CORBA::Core::PolicyImpl <IMPLICIT_ACTIVATION_POLICY_ID>::create (value);
	}

	static ServantRetentionPolicy::_ref_type create_servant_retention_policy (ServantRetentionPolicyValue value)
	{
		return CORBA::Core::PolicyImpl <SERVANT_RETENTION_POLICY_ID>::create (value);
	}

	static RequestProcessingPolicy::_ref_type create_request_processing_policy (RequestProcessingPolicyValue value)
	{
		return CORBA::Core::PolicyImpl <REQUEST_PROCESSING_POLICY_ID>::create (value);
	}

	// POA attributes
	virtual IDL::String the_name () const
	{
		IDL::String name;
		if (name_)
			name = *name_;
		return name;
	}

	POA::_ref_type the_parent () const NIRVANA_NOEXCEPT
	{
		if (parent_)
			return parent_->_this ();
		else
			return POA::_nil ();
	}
	
	POAList the_children () const
	{
		POAList list;
		list.reserve (children_.size ());
		for (auto it = children_.begin (); it != children_.end (); ++it) {
			list.push_back (it->second->_this ());
		}
		return list;
	}

	PortableServer::POAManager::_ref_type the_POAManager () const;

	AdapterActivator::_ref_type the_activator () const
	{
		return the_activator_;
	}

	void the_activator (AdapterActivator::_ptr_type a)
	{
		the_activator_ = a;
	}

	// Servant Manager registration:

	virtual ServantManager::_ref_type get_servant_manager ()
	{
		throw WrongPolicy ();
	}

	virtual void set_servant_manager (ServantManager::_ptr_type imgr)
	{
		throw WrongPolicy ();
	}

	// Operations for the USE_DEFAULT_SERVANT policy

	virtual CORBA::Object::_ref_type get_servant ()
	{
		throw WrongPolicy ();
	}

	virtual void set_servant (CORBA::Object::_ptr_type proxy)
	{
		throw WrongPolicy ();
	}

	// Object activation and deactivation
	virtual ObjectId activate_object (CORBA::Object::_ptr_type p_servant) = 0;
	virtual void activate_object_with_id (const ObjectId& oid, CORBA::Object::_ptr_type p_servant) = 0;
	virtual void deactivate_object (const ObjectId& oid) = 0;

	// Reference creation operations
	virtual CORBA::Object::_ref_type create_reference (const CORBA::RepositoryId& intf) = 0;
	virtual CORBA::Object::_ref_type create_reference_with_id (const ObjectId& oid, const CORBA::RepositoryId& intf) = 0;

	// Identity mapping operations:
	virtual ObjectId servant_to_id (CORBA::Object::_ptr_type p_servant);
	virtual CORBA::Object::_ref_type servant_to_reference (CORBA::Object::_ptr_type p_servant);
	virtual CORBA::Object::_ref_type reference_to_servant (CORBA::Object::_ptr_type reference);
	virtual ObjectId reference_to_id (CORBA::Object::_ptr_type reference);
	virtual CORBA::Object::_ref_type id_to_reference (const ObjectId& oid) = 0;

	// Additional attributes

	virtual CORBA::OctetSeq id () const = 0;
	
	PortableServer::POAManagerFactory::_ref_type the_POAManagerFactory ();

	// Internal
	static bool implicit_activation (POA::_ptr_type poa) NIRVANA_NOEXCEPT
	{
		if (poa) {
			const POA_Base* impl = get_implementation (CORBA::Core::local2proxy (poa));
			if (impl)
				return impl->implicit_activation ();
		}
		return false;
	}

	void invoke (const RequestRef& request);

	virtual void serve (const RequestRef& request) = 0;

	static POA_Base* get_implementation (const CORBA::Core::ProxyLocal* proxy);

	// Entry point vector overrides
	static CORBA::Internal::Interface* _s_get_servant (CORBA::Internal::Bridge <POA>* _b, Interface* _env);
	static void _s_set_servant (CORBA::Internal::Bridge <POA>* _b, Interface* p_servant, Interface* _env);
	static CORBA::Internal::Type <ObjectId>::ABI_ret _s_activate_object (
		CORBA::Internal::Bridge <POA>* _b, Interface* p_servant,
		Interface* env);
	static void _s_activate_object_with_id (
		CORBA::Internal::Bridge <POA>* _b, CORBA::Internal::Type <ObjectId>::ABI_in oid, Interface* p_servant,
		Interface* env);
	static CORBA::Internal::Type <ObjectId>::ABI_ret _s_servant_to_id (
		CORBA::Internal::Bridge <POA>* _b, Interface* p_servant,
		Interface* env);
	static Interface* _s_servant_to_reference (
		CORBA::Internal::Bridge <POA>* _b, Interface* servant,
		Interface* env);
	static Interface* _s_reference_to_servant (
		CORBA::Internal::Bridge <POA>* _b, Interface* reference,
		Interface* env);
	static Interface* _s_id_to_servant (
		CORBA::Internal::Bridge <POA>* _b, CORBA::Internal::Type <ObjectId>::ABI_in oid,
		Interface* env);

	virtual ~POA_Base ();

	typedef CORBA::servant_reference <POA_Base> POA_Ref;

protected:
	POA_Base (POA_Base* parent, const IDL::String* name, CORBA::servant_reference <POAManager>&& manager);

	virtual bool implicit_activation () const NIRVANA_NOEXCEPT = 0;

	void get_path (AdapterPath& path, size_t size = 0) const;
	bool check_path (const AdapterPath& path, AdapterPath::const_iterator it) const
		NIRVANA_NOEXCEPT;

	bool check_path (const AdapterPath& path) const NIRVANA_NOEXCEPT
	{
		return check_path (path, path.end ());
	}

	CORBA::Object::_ref_type servant_to_reference_nothrow (CORBA::Object::_ptr_type p_servant);

	void serve (const RequestRef& request, CORBA::Core::ProxyObject& proxy);

private:
	void on_request_finish () NIRVANA_NOEXCEPT;

private:
	// Children map.
	typedef Nirvana::Core::MapUnorderedStable <IDL::String, POA_Ref,
		std::hash <IDL::String>, std::equal_to <IDL::String>,
		Nirvana::Core::UserAllocator <std::pair <IDL::String, POA_Ref> > > Children;

	POA_Base* parent_;
	const IDL::String* name_;
	Children children_;
	CORBA::servant_reference <POAManager> the_POAManager_;
	AdapterActivator::_ref_type the_activator_;
	unsigned int request_cnt_;
	bool destroyed_;
	Nirvana::Core::Event destroy_completed_;

	static const int32_t SIGNATURE = 'POA_';
	const int32_t signature_;
};

}
}

#endif
