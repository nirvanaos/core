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
#ifndef NIRVANA_ORB_CORE_PROXYMANAGER_H_
#define NIRVANA_ORB_CORE_PROXYMANAGER_H_
#pragma once

#include "CORBA/CORBA.h"
#include "../Synchronized.h"
#include "../Array.h"
#include "../SharedObject.h"
#include "../SharedAllocator.h"
#include <CORBA/Proxy/InterfaceMetadata.h>
#include <CORBA/Proxy/ProxyFactory.h>
#include <CORBA/Proxy/IOReference.h>
#include <CORBA/DynamicServant.h>

namespace CORBA {
namespace Core {

/// The implicit object reference operations _non_existent, _is_a, _repository_id and _get_interface
/// may be invoked using DII. No other implicit object reference operations may be invoked via DII.
/// Only _non_existent operation can cause the call to servant.
/// Object operation indexes.
enum class ObjectOp : UShort
{
	GET_INTERFACE,
	IS_A,
	NON_EXISTENT,
	REPOSITORY_ID
};

/// This array contains metadata of the operations:
/// - _get_interface
/// - _is_a
/// - _non_existent
/// - _repository_id
typedef Internal::Operation OperationsDII [4];

/// \brief Base for all proxies.
class ProxyManager :
	public Nirvana::Core::SharedObject,
	public Internal::Bridge <Internal::IOReference>,
	public Internal::Bridge <Object>,
	public Internal::Bridge <AbstractBase>
{
public:
	Internal::Bridge <Object>* get_object (Internal::String_in iid)
	{
		if (Internal::RepId::check (Internal::RepIdOf <Object>::id, iid) != Internal::RepId::COMPATIBLE)
			Nirvana::throw_INV_OBJREF ();
		return static_cast <Internal::Bridge <Object>*> (this);
	}

	Internal::Bridge <AbstractBase>* get_abstract_base (Internal::String_in iid) NIRVANA_NOEXCEPT
	{
		if (Internal::RepId::check (Internal::RepIdOf <AbstractBase>::id, iid) != Internal::RepId::COMPATIBLE)
			Nirvana::throw_INV_OBJREF ();
		return static_cast <Internal::Bridge <AbstractBase>*> (this);
	}

	Object::_ptr_type get_proxy () NIRVANA_NOEXCEPT
	{
		return &static_cast <Object&> (static_cast <Internal::Bridge <Object>&> (*this));
	}

	// Query interface
	Internal::Interface* _query_interface (const IDL::String& type_id) const
	{
		if (type_id.empty ())
			return primary_interface_->proxy;
		else {
			const InterfaceEntry* ie = find_interface (type_id);
			if (ie)
				return ie->proxy;
			else
				return nullptr;
		}
	}

	// Object operations

	static InterfaceDef::_ref_type _get_interface ()
	{
		SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, nullptr);
		return get_interface ();
		SYNC_END ();
	}
	
	Boolean _is_a (const IDL::String& type_id) const
	{
		IDL::String tmp (type_id);
		SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, nullptr);
		return is_a (tmp);
		SYNC_END ();
	}

	Boolean _non_existent ()
	{
		Internal::IORequest::_ref_type rq = ior ()->create_request (_make_op_idx ((UShort)ObjectOp::NON_EXISTENT));
		rq->invoke ();
		Boolean _ret;
		Internal::Type <Boolean>::unmarshal (rq, _ret);
		rq->unmarshal_end ();
		return _ret;
	}

	Boolean _is_equivalent (Object::_ptr_type other_object) const
	{
		return &static_cast <const Internal::Bridge <Object>&> (*this) == &other_object;
	}

	ULong _hash (ULong maximum) const
	{
		return (ULong)((uintptr_t)this 
			- ::Nirvana::Core::Port::Memory::query (nullptr, Nirvana::Memory::QueryParam::ALLOCATION_SPACE_BEGIN)
			) % maximum;
	}

	void _create_request (Context::_ptr_type ctx, const IDL::String& operation, NVList::_ptr_type arg_list,
		NamedValue::_ptr_type result, Request::_ref_type& request, Flags req_flags)
	{
		throw NO_IMPLEMENT ();
	}

	void _create_request (Context::_ptr_type ctx, const IDL::String& operation, NVList::_ptr_type arg_list,
		NamedValue::_ptr_type result, const ExceptionList& exclist, const ContextList& ctxlist,
		Request::_ref_type& request, Flags req_flags)
	{
		throw NO_IMPLEMENT ();
	}

	Policy::_ref_type _get_policy (PolicyType policy_type)
	{
		throw NO_IMPLEMENT ();
	}

	DomainManagersList _get_domain_managers ()
	{
		throw NO_IMPLEMENT ();
	}

	Object::_ref_type _set_policy_overrides (const PolicyList& policies, SetOverrideType set_or_add)
	{
		throw NO_IMPLEMENT ();
	}

	Policy::_ref_type _get_client_policy (PolicyType type)
	{
		throw NO_IMPLEMENT ();
	}

	PolicyList _get_policy_overrides (const PolicyTypeSeq& types)
	{
		throw NO_IMPLEMENT ();
	}

	Boolean _validate_connection (PolicyList& inconsistent_policies)
	{
		throw NO_IMPLEMENT ();
	}

	IDL::String _repository_id () const
	{
		//Nirvana::Core::ExecDomain::yield ();
		return repository_id ();
	}
	
	Object::_ref_type _get_component ()
	{
		return Object::_nil ();
	}

	// Abstract base implementation

	Object::_ref_type _to_object () NIRVANA_NOEXCEPT
	{
		return get_proxy ();
	}

	ValueBase::_ref_type _to_value () NIRVANA_NOEXCEPT
	{
		return nullptr;
	}

	Internal::IOReference::OperationIndex find_operation (const IDL::String& name) const;

protected:
	ProxyManager (const Internal::Bridge <Internal::IOReference>::EPV& epv_ior,
		const Internal::Bridge <Object>::EPV& epv_obj, const Internal::Bridge <AbstractBase>::EPV& epv_ab,
		Internal::String_in primary_iid, const OperationsDII& object_ops, void* object_impl);

	~ProxyManager ();

	Internal::IOReference::_ptr_type ior ()
	{
		return &static_cast <Internal::IOReference&> (static_cast <Internal::Bridge <Internal::IOReference>&> (*this));
	}

	struct InterfaceEntry
	{
		const Char* iid;
		size_t iid_len;
		Internal::Interface* proxy;
		Internal::DynamicServant::_ptr_type deleter;
		Internal::Interface::_ptr_type implementation;
		Internal::CountedArray <Internal::Operation> operations;
	};

	struct OperationEntry
	{
		const Char* name;
		size_t name_len;
		Internal::IOReference::OperationIndex idx;
	};

	Nirvana::Core::Array <InterfaceEntry, Nirvana::Core::SharedAllocator>& interfaces ()
	{
		return interfaces_;
	}

	const InterfaceEntry* find_interface (Internal::String_in iid) const;

	UShort object_itf_idx () const
	{
		return object_itf_idx_;
	}

	Internal::IOReference::OperationIndex _make_op_idx (UShort op_idx) const
	{
		return Internal::IOReference::OperationIndex (object_itf_idx_, op_idx);
	}

	static InterfaceDef::_ref_type get_interface ();

	Boolean is_a (const IDL::String& type_id) const
	{
		return find_interface (type_id) != nullptr;
	}

	IDL::String repository_id () const
	{
		return IDL::String (primary_interface_->iid, primary_interface_->iid_len);
	}

	void serve_object_request (ObjectOp op, Internal::IORequest::_ptr_type rq);

	// Input parameter metadata for `is_a` operation.
	static const Internal::Parameter is_a_param_;

	// Implicit operation names
	static const Char op_get_interface_ [];
	static const Char op_is_a_ [];
	static const Char op_non_existent_ [];
	static const Char op_repository_id_ [];

private:
	struct IEPred;
	struct OEPred;

	void create_proxy (InterfaceEntry& ie);
	void create_proxy (Internal::ProxyFactory::_ptr_type pf, InterfaceEntry& ie);

	static void check_metadata (const Internal::InterfaceMetadata* metadata, Internal::String_in primary);
	static void check_parameters (Internal::CountedArray <Internal::Parameter> parameters);
	static void check_type_code (TypeCode::_ptr_type tc);
	
	template <class It, class Pr>
	static bool is_unique (It begin, It end, Pr pred)
	{
		if (begin != end) {
			It prev = begin;
			for (It p = begin; ++p != end; prev = p) {
				if (!pred (*prev, *p))
					return false;
			}
		}
		return true;
	}

private:
	Nirvana::Core::Array <InterfaceEntry, Nirvana::Core::SharedAllocator> interfaces_;
	Nirvana::Core::Array <OperationEntry, Nirvana::Core::SharedAllocator> operations_;

	const InterfaceEntry* primary_interface_;
	UShort object_itf_idx_;
};

}
}

#endif
