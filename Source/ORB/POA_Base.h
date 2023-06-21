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
#include "PortableServer_policies.h"
#include "ServantProxyObject.h"
#include "ServantProxyLocal.h"
#include "MapUnorderedStable.h"
#include "RequestInPOA.h"
#include "Services.h"
#include "ReferenceLocal.h"
#include "PolicyMap.h"
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
class POA_Root;
class POA_Base;

typedef Nirvana::Core::Ref <CORBA::Core::RequestInPOA> RequestRef;

typedef CORBA::servant_reference <POA_Base> POA_Ref;

typedef POA_Ref (*POA_Factory) (POA_Base* parent, const IDL::String* name,
	CORBA::servant_reference <POAManager>&& manager,
	CORBA::servant_reference <CORBA::Core::PolicyMapShared>&& policy_map);

struct POA_Policies
{
	LifespanPolicyValue lifespan;
	IdUniquenessPolicyValue id_uniqueness;
	IdAssignmentPolicyValue id_assignment;
	ImplicitActivationPolicyValue implicit_activation;
	ServantRetentionPolicyValue servant_retention;
	RequestProcessingPolicyValue request_processing;

	void set_values (const CORBA::PolicyList& policies, CORBA::Core::PolicyMapRef& object_policies)
	{
		unsigned mask = 0;
		*this = default_;
		int DGC_policy_idx = -1;
		for (auto it = policies.begin (); it != policies.end (); ++it) {
			CORBA::Policy::_ptr_type policy = *it;
			CORBA::PolicyType type = policy->policy_type ();
			if (LIFESPAN_POLICY_ID <= type && type <= REQUEST_PROCESSING_POLICY_ID) {
				unsigned bit = 1 << (type - LIFESPAN_POLICY_ID);
				if (mask & bit)
					throw POA::InvalidPolicy (CORBA::UShort (it - policies.begin ()));
				mask |= bit;
			}
			switch (type) {
				case THREAD_POLICY_ID:
					if (ThreadPolicyValue::ORB_CTRL_MODEL != ThreadPolicy::_narrow (policy)->value ())
						throw POA::InvalidPolicy (CORBA::UShort (it - policies.begin ()));
					break;
				case LIFESPAN_POLICY_ID:
					lifespan = LifespanPolicy::_narrow (policy)->value ();
					break;
				case ID_UNIQUENESS_POLICY_ID:
					id_uniqueness = IdUniquenessPolicy::_narrow (policy)->value ();
					break;
				case ID_ASSIGNMENT_POLICY_ID:
					id_assignment = IdAssignmentPolicy::_narrow (policy)->value ();
					break;
				case IMPLICIT_ACTIVATION_POLICY_ID:
					implicit_activation = ImplicitActivationPolicy::_narrow (policy)->value ();
					break;
				case SERVANT_RETENTION_POLICY_ID:
					servant_retention = ServantRetentionPolicy::_narrow (policy)->value ();
					break;
				case REQUEST_PROCESSING_POLICY_ID:
					request_processing = RequestProcessingPolicy::_narrow (policy)->value ();
					break;
				default:
					if (FT::HEARTBEAT_ENABLED_POLICY == type)
						DGC_policy_idx = int (it - policies.begin ());
					if (!object_policies)
						object_policies = CORBA::make_reference <CORBA::Core::PolicyMapShared> ();
					if (!object_policies->insert (policy))
						throw POA::InvalidPolicy (CORBA::UShort (it - policies.begin ()));
			}
		}
		// DGC requires RETAIN policy
		if (DGC_policy_idx >= 0 && servant_retention != ServantRetentionPolicyValue::RETAIN)
			throw POA::InvalidPolicy ((CORBA::UShort)DGC_policy_idx);
	}

	bool operator < (const POA_Policies& rhs) const noexcept
	{
		typedef CORBA::Internal::ABI_enum Int;
		return std::lexicographical_compare (
			(const Int*)this, (const Int*)(this + 1),
			(const Int*)&rhs, (const Int*)(&rhs + 1));
	}

	static const POA_Policies default_;
};

struct POA_FactoryEntry
{
	POA_Policies policies;
	POA_Factory factory;
};

inline
bool operator < (const POA_FactoryEntry& l, const POA_Policies& r) noexcept
{
	return l.policies < r;
}

inline
bool operator < (const POA_Policies& l, const POA_FactoryEntry& r) noexcept
{
	return l < r.policies;
}

// POA implementation always operates the reference to Object interface (proxy),
// not to ServantBase.
// Release of the ServantBase reference must be performed in the ServantBase
// synchronization context, so the reference counting of the ServantBase is
// responsibility of the proxy.
// The POA never communicates with user ServantBase directly.
class NIRVANA_NOVTABLE POA_Base :
	public CORBA::servant_traits <POA>::Servant <POA_Base>
{
	static const TimeBase::TimeT ADAPTER_ACTIVATION_DEADLINE = 1 * TimeBase::MILLISECOND;

public:
	// POA creation and destruction

	inline POA::_ref_type create_POA (const IDL::String& adapter_name,
		PortableServer::POAManager::_ptr_type a_POAManager, const CORBA::PolicyList& policies);

	POA::_ref_type find_POA (const IDL::String& adapter_name, bool activate_it)
	{
		POA_Ref p = find_child (adapter_name, activate_it);
		if (p)
			return p->_this ();
		else
			throw AdapterNonExistent ();
	}

	POA_Ref find_child (const IDL::String& adapter_name, bool activate_it);

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

	void destroy (bool etherealize_objects) noexcept
	{
		destroy_internal (etherealize_objects);
		if (!request_cnt_)
			destroy_completed_.signal ();
	}

	bool is_destroyed () const noexcept
	{
		return destroyed_;
	}

	virtual void destroy_internal (bool etherealize_objects) noexcept;
	virtual void etherealize_objects () noexcept {}
	virtual void etherialize (const ObjectId& oid, CORBA::Core::ServantProxyObject& proxy,
		bool cleanup_in_progress) noexcept {};

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

	static IdAssignmentPolicy::_ref_type create_id_assignment_policy (IdAssignmentPolicyValue value)
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

	POA::_ref_type the_parent () const noexcept
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
			list.push_back (it->second.get_if_constructed ()->_this ());
		}
		return list;
	}

	inline PortableServer::POAManager::_ref_type the_POAManager () const;

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

	virtual void set_servant (CORBA::Object::_ptr_type p_servant)
	{
		throw WrongPolicy ();
	}

	// Object activation and deactivation
	ObjectId activate_object (CORBA::Object::_ptr_type p_servant)
	{
		CORBA::Core::ServantProxyObject* proxy = CORBA::Core::object2proxy (p_servant);
		if (!proxy)
			throw CORBA::BAD_PARAM ();
		ObjectId oid;
		activate_object (*proxy, oid);
		return oid;
	}

	void activate_object (CORBA::Core::ServantProxyObject& proxy, ObjectId& oid, unsigned flags = 0);
	CORBA::Core::ReferenceLocalRef activate_object (CORBA::Core::ServantProxyObject& proxy,
		unsigned flags = 0);

	void activate_object_with_id (ObjectId& oid, CORBA::Object::_ptr_type p_servant)
	{
		CORBA::Core::ServantProxyObject* proxy = CORBA::Core::object2proxy (p_servant);
		if (!proxy)
			throw CORBA::BAD_PARAM ();
		check_object_id (oid);
		activate_object (ObjectKey (*this, std::move (oid)), false, *proxy, 0);
	}

	virtual CORBA::Core::ReferenceLocalRef activate_object (ObjectKey&& key, bool unique,
		CORBA::Core::ServantProxyObject& proxy, unsigned flags);

	virtual void activate_object (CORBA::Core::ReferenceLocal& ref, CORBA::Core::ServantProxyObject& proxy);

	virtual void deactivate_object (ObjectId& oid);
	virtual CORBA::servant_reference <CORBA::Core::ServantProxyObject> deactivate_object (
		CORBA::Core::ReferenceLocal& ref);

	inline static void implicit_activate (POA::_ptr_type adapter, CORBA::Core::ServantProxyObject& proxy);
	virtual void implicit_deactivate (CORBA::Core::ReferenceLocal& ref,
		CORBA::Core::ServantProxyObject& proxy) noexcept;

	// Reference creation operations
	virtual CORBA::Object::_ref_type create_reference (CORBA::Internal::String_in iid);
	
	CORBA::Core::ReferenceLocalRef create_reference (CORBA::Internal::String_in iid,
		unsigned flags);

	CORBA::Object::_ref_type create_reference_with_id (ObjectId& oid, CORBA::Internal::String_in iid)
	{
		return CORBA::Object::_ref_type (
			create_reference (ObjectKey (*this, std::move (oid)), iid)->get_proxy ());
	}

	virtual CORBA::Core::ReferenceLocalRef create_reference (ObjectKey&& key,
		CORBA::Internal::String_in iid);

	CORBA::Core::ReferenceLocalRef create_reference (ObjectKey&& key,
		CORBA::Internal::String_in intf, unsigned flags);

	// Identity mapping operations:
	ObjectId servant_to_id (CORBA::Object::_ptr_type p_servant)
	{
		CORBA::Core::ServantProxyObject* proxy = CORBA::Core::object2proxy (p_servant);
		if (!proxy)
			throw CORBA::BAD_PARAM ();
		return servant_to_id (*proxy);
	}

	virtual ObjectId servant_to_id (CORBA::Core::ServantProxyObject& proxy);
	virtual ObjectId servant_to_id_default (CORBA::Core::ServantProxyObject& proxy, bool not_found);

	CORBA::Object::_ref_type servant_to_reference (CORBA::Object::_ptr_type p_servant)
	{
		CORBA::Core::ServantProxyObject* proxy = CORBA::Core::object2proxy (p_servant);
		if (!proxy)
			throw CORBA::BAD_PARAM ();
		return servant_to_reference (*proxy);
	}

	virtual CORBA::Object::_ref_type servant_to_reference (CORBA::Core::ServantProxyObject& proxy);
	virtual CORBA::Object::_ref_type servant_to_reference_default (CORBA::Core::ServantProxyObject& proxy, bool not_found);

	virtual CORBA::Object::_ref_type reference_to_servant (CORBA::Object::_ptr_type reference);
	virtual CORBA::Object::_ref_type reference_to_servant_default (bool not_active);

	ObjectId reference_to_id (CORBA::Object::_ptr_type reference);

	virtual CORBA::Object::_ref_type id_to_servant (ObjectId& oid);
	virtual CORBA::Object::_ref_type id_to_servant_default (bool not_active);

	virtual CORBA::Object::_ref_type id_to_reference (ObjectId& oid);

	// Additional attributes

	virtual CORBA::OctetSeq id () const = 0;

	inline
	static PortableServer::POAManagerFactory::_ref_type the_POAManagerFactory () noexcept;

	static bool implicit_activation (POA::_ptr_type poa) noexcept
	{
		if (poa) {
			const POA_Base* impl = get_implementation (CORBA::Core::local2proxy (poa));
			if (impl)
				return impl->implicit_activation ();
		}
		return false;
	}

	inline void invoke (const RequestRef& request);
	void serve_request (const RequestRef& request);

	static POA_Base* get_implementation (const CORBA::Core::ServantProxyLocal* proxy);

	// Entry point vector overrides
	static CORBA::Internal::Interface* _s_get_servant (CORBA::Internal::Bridge <POA>* _b,
		CORBA::Internal::Interface* _env);

	static void _s_set_servant (CORBA::Internal::Bridge <POA>* _b,
		CORBA::Internal::Interface* p_servant, CORBA::Internal::Interface* _env);

	static CORBA::Internal::Type <ObjectId>::ABI_ret _s_activate_object (CORBA::Internal::Bridge <POA>* _b,
		CORBA::Internal::Interface* p_servant, CORBA::Internal::Interface* env);

	static void _s_activate_object_with_id (CORBA::Internal::Bridge <POA>* _b,
		CORBA::Internal::Type <ObjectId>::ABI_in oid, CORBA::Internal::Interface* p_servant,
		CORBA::Internal::Interface* env);

	static CORBA::Internal::Type <ObjectId>::ABI_ret _s_servant_to_id (CORBA::Internal::Bridge <POA>* _b,
		CORBA::Internal::Interface* p_servant, CORBA::Internal::Interface* env);

	static CORBA::Internal::Interface* _s_servant_to_reference (	CORBA::Internal::Bridge <POA>* _b,
		CORBA::Internal::Interface* servant, CORBA::Internal::Interface* env);

	static CORBA::Internal::Interface* _s_reference_to_servant (CORBA::Internal::Bridge <POA>* _b,
		CORBA::Internal::Interface* reference, CORBA::Internal::Interface* env);

	static CORBA::Internal::Interface* _s_id_to_servant (CORBA::Internal::Bridge <POA>* _b,
		CORBA::Internal::Type <ObjectId>::ABI_in oid, CORBA::Internal::Interface* env);

	virtual ~POA_Base ();

	virtual ObjectId generate_object_id ();
	virtual void check_object_id (const ObjectId& oid);
	void get_path (AdapterPath& path, size_t size = 0) const;

	bool check_path (const AdapterPath& path) const noexcept
	{
		return check_path (path, path.end ());
	}

protected:
	friend class CORBA::Core::ReferenceLocal;

	static POA_Root& root () noexcept
	{
		assert (root_);
		return *root_;
	}

	static CORBA::Object::_ref_type get_root ()
	{
		assert ((CORBA::Object::_ref_type&)root_object_);
		return root_object_;
	}

	POA_Base () :
		signature_ (0)
	{
		NIRVANA_UNREACHABLE_CODE ();
	}

	POA_Base (POA_Base* parent, const IDL::String* name,
		CORBA::servant_reference <POAManager>&& manager,
		CORBA::Core::PolicyMapRef&& object_policies);

	virtual CORBA::Core::PolicyMapShared* get_policies (unsigned flags) const noexcept
	{
		return object_policies_;
	}

	virtual unsigned get_flags (unsigned flags) const noexcept
	{
		return flags;
	}

	virtual bool implicit_activation () const noexcept;

	bool check_path (const AdapterPath& path, AdapterPath::const_iterator it) const
		noexcept;

	virtual void serve_request (const RequestRef& request, CORBA::Core::ReferenceLocal& reference);
	virtual void serve_default (const RequestRef& request, CORBA::Core::ReferenceLocal& reference);
	void serve_request (const RequestRef& request, CORBA::Core::ReferenceLocal& reference, CORBA::Core::ServantProxyObject& proxy);

private:
	void on_request_finish () noexcept;
	
protected:
	static Nirvana::Core::StaticallyAllocated <CORBA::Object::_ref_type> root_object_;
	static POA_Root* root_;

	CORBA::servant_reference <POAManager> the_POAManager_;
	CORBA::Core::PolicyMapRef object_policies_;

private:
	// Children map.
	typedef Nirvana::Core::MapUnorderedStable <IDL::String, Nirvana::Core::WaitableRef <POA_Ref>,
		std::hash <IDL::String>, std::equal_to <IDL::String>,
		Nirvana::Core::UserAllocator> Children;

	POA_Base* parent_;
	const IDL::String* name_;
	Children children_;
	AdapterActivator::_ref_type the_activator_;
	unsigned int request_cnt_;
	bool destroyed_;
	Nirvana::Core::Event destroy_completed_;

	static const int32_t SIGNATURE = 'POA_';
	const int32_t signature_;

	static const size_t FACTORY_COUNT;
	static const POA_FactoryEntry factories_ [];
};

}
}

#endif
