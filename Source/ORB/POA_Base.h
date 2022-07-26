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
#include "ServantBase.h"
#include <unordered_map>
#include "HashOctetSeq.h"
#include "IDL/PortableServer_s.h"
#include "LocalObject.h"

namespace PortableServer {
namespace Core {

// POA implementation always operates the reference to Object interface (proxy),
// not to ServantBase.
// Release of the ServantBase reference must be performed in the ServantBase
// synchronization context, so the reference counting of the ServantBase is
// responsibility of proxy.
// The POA never communicates with user ServantBase directly.

// Active Object Map (AOM)
// TODO: Use another fast implementation with the pointer stability.
typedef ObjectId AOM_Key;
typedef CORBA::Object::_ref_type AOM_Val;

typedef std::unordered_map <AOM_Key, AOM_Val,
	std::hash <AOM_Key>, std::equal_to <AOM_Key>, Nirvana::Core::UserAllocator
	<std::pair <AOM_Key, AOM_Val> > > AOM;

class NIRVANA_NOVTABLE POA_Base :
	public CORBA::servant_traits <POA>::Servant <POA_Base>
{
protected:
	// Children map.
	// TODO: Use another fast implementation with the pointer stability.
	typedef std::unordered_map <IDL::String, POA::_ref_type> Children;

public:
	// POA creation and destruction

	POA::_ref_type create_POA (const IDL::String& adapter_name,
		POAManager::_ptr_type a_POAManager, const CORBA::PolicyList& policies)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	POA::_ref_type find_POA (const IDL::String& adapter_name, bool activate_it)
	{
		auto it = children_.find (adapter_name);
		if (it == children_.end ()) {
			if (activate_it && the_activator_) {
				bool created = false;
				try {
					created = the_activator_->unknown_adapter (_this (), adapter_name);
				} catch (const CORBA::SystemException&) {
					throw CORBA::OBJ_ADAPTER (1);
				}
				if (created)
					it = children_.find (adapter_name);
				else
					throw CORBA::OBJECT_NOT_EXIST (2);
			}
		}
		if (it == children_.end ())
			throw AdapterNonExistent ();
		else
			return it->second;
	}

	virtual void destroy (bool etherealize_objects, bool wait_for_completion)
	{
		Children tmp (std::move (children_));
		while (!tmp.empty ()) {
			tmp.begin ()->second->destroy (etherealize_objects, wait_for_completion);
		}
	}

	// Factories for Policy objects

	static ThreadPolicy::_ref_type create_thread_policy (ThreadPolicyValue value)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static LifespanPolicy::_ref_type create_lifespan_policy (LifespanPolicyValue value)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static IdUniquenessPolicy::_ref_type create_id_uniqueness_policy (IdUniquenessPolicyValue value)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static IdAssignmentPolicy::_ref_type  create_id_assignment_policy (IdAssignmentPolicyValue value)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static ImplicitActivationPolicy::_ref_type create_implicit_activation_policy (ImplicitActivationPolicyValue value)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static ServantRetentionPolicy::_ref_type create_servant_retention_policy (ServantRetentionPolicyValue value)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static RequestProcessingPolicy::_ref_type create_request_processing_policy (RequestProcessingPolicyValue value)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	// POA attributes
	virtual IDL::String the_name () const = 0;
	virtual POA::_ref_type the_parent () const NIRVANA_NOEXCEPT = 0;
	
	POAList the_children () const
	{
		POAList list;
		list.reserve (children_.size ());
		for (auto it = children_.begin (); it != children_.end (); ++it) {
			list.push_back (it->second);
		}
		return list;
	}

	POAManager::_ref_type the_POAManager () const
	{
		return the_POAManager_;
	}

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

	// operations for the USE_DEFAULT_SERVANT policy

	virtual CORBA::Object::_ref_type get_servant ()
	{
		throw WrongPolicy ();
	}

	virtual void set_servant (CORBA::Object::_ptr_type proxy)
	{
		throw WrongPolicy ();
	}

	// object activation and deactivation
	virtual ObjectId activate_object (CORBA::Object::_ptr_type p_servant) = 0;
	virtual void deactivate_object (const ObjectId& oid) = 0;

	// reference creation operations
	CORBA::Object::_ref_type create_reference (const CORBA::RepositoryId& intf)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	CORBA::Object::_ref_type create_reference_with_id (const ObjectId& oid, const CORBA::RepositoryId& intf)
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	// Identity mapping operations:
	virtual ObjectId servant_to_id (CORBA::Object::_ptr_type p_servant) = 0;
	virtual CORBA::Object::_ref_type servant_to_reference (CORBA::Object::_ptr_type p_servant) = 0;
	virtual CORBA::Object::_ref_type reference_to_servant (CORBA::Object::_ptr_type reference);
	virtual ObjectId reference_to_id (CORBA::Object::_ptr_type reference);
	virtual CORBA::Object::_ref_type id_to_reference (const ObjectId& oid) = 0;

	// Additional attributes
	virtual CORBA::OctetSeq id () const = 0;
	
	POAManagerFactory::_ref_type the_POAManagerFactory ()
	{
		throw CORBA::NO_IMPLEMENT ();
	}

	static const CORBA::Core::LocalObject* get_proxy (POA::_ptr_type poa) NIRVANA_NOEXCEPT;
	static POA_Base* get_implementation (POA::_ptr_type poa) NIRVANA_NOEXCEPT;

	static bool implicit_activation (POA::_ptr_type poa) NIRVANA_NOEXCEPT
	{
		if (poa) {
			const POA_Base* impl = get_implementation (poa);
			if (impl)
				return impl->implicit_activation ();
		}
		return false;
	}

	// Entry point vector overrides
	static CORBA::Internal::Interface* _s_get_servant (CORBA::Internal::Bridge <POA>* _b, Interface* _env);
	static void _s_set_servant (CORBA::Internal::Bridge <POA>* _b, Interface* p_servant, Interface* _env);
	static CORBA::Internal::Type <ObjectId>::ABI_ret _s_activate_object (
		CORBA::Internal::Bridge <POA>* _b, Interface* p_servant,
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

protected:
	POA_Base (POAManager::_ptr_type manager) :
		the_POAManager_ (manager),
		signature_ (SIGNATURE)
	{}

	// Nirvana extension
	virtual bool implicit_activation () const NIRVANA_NOEXCEPT = 0;

protected:
	Children children_;
	POAManager::_ref_type the_POAManager_;
	AdapterActivator::_ref_type the_activator_;

private:
	static const int32_t SIGNATURE = 'POA_';
	const int32_t signature_;
};

}
}

#endif
