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
#include "../HeapAllocator.h"
#include <CORBA/Proxy/InterfaceMetadata.h>
#include <CORBA/Proxy/ProxyFactory.h>
#include <CORBA/DynamicServant.h>
#include <CORBA/AbstractBase_s.h>
#include <CORBA/Object_s.h>
#include <CORBA/Proxy/IOReference_s.h>
#include "offset_ptr.h"

namespace PortableServer {
namespace Core {
class POA_Base;
}
}

namespace CORBA {
namespace Core {

class Reference;
typedef servant_reference <Reference> ReferenceRef;
class ReferenceLocal;
typedef servant_reference <ReferenceLocal> ReferenceLocalRef;

class StreamOut;

/// \brief Base for all proxies.
class NIRVANA_NOVTABLE ProxyManager :
	public Nirvana::Core::SharedObject,
	public Internal::ServantTraits <ProxyManager>,
	public Internal::LifeCycleRefCnt <ProxyManager>,
	public Internal::InterfaceImplBase <ProxyManager, Internal::IOReference>,
	public Internal::InterfaceImplBase <ProxyManager, Object>,
	public Internal::InterfaceImplBase <ProxyManager, AbstractBase>
{
protected:
	virtual void _add_ref () = 0;
	virtual void _remove_ref () noexcept = 0;
	template <class> friend class Nirvana::Core::Ref;
	template <class> friend class CORBA::servant_reference;
	friend class Internal::LifeCycleRefCnt <ProxyManager>;

public:
	// get_heap () returns SyncDomain heap for SyncDomain
	// and shared heap for free sync context.

	void* operator new (size_t cb)
	{
		return get_heap ().allocate (nullptr, cb, 0);
	}

	void operator delete (void* p, size_t cb)
	{
		get_heap ().release (p, cb);
	}

	void* operator new (size_t cb, void* place)
	{
		return place;
	}

	void operator delete (void*, void*)
	{}

	// IOReference operations

	Internal::Bridge <Object>* get_object (Internal::String_in iid)
	{
		if (Internal::RepId::check (Internal::RepIdOf <Object>::id, iid) != Internal::RepId::COMPATIBLE)
			Nirvana::throw_INV_OBJREF ();
		return static_cast <Internal::Bridge <Object>*> (this);
	}

	Internal::Bridge <AbstractBase>* get_abstract_base (Internal::String_in iid) noexcept
	{
		if (Internal::RepId::check (Internal::RepIdOf <AbstractBase>::id, iid) != Internal::RepId::COMPATIBLE)
			Nirvana::throw_INV_OBJREF ();
		return static_cast <Internal::Bridge <AbstractBase>*> (this);
	}

	virtual Internal::IORequest::_ref_type create_request (OperationIndex op, unsigned flags,
		Internal::RequestCallback::_ptr_type callback);

	// Get Object proxy
	Object::_ptr_type get_proxy () noexcept
	{
		return &static_cast <Object&> (static_cast <Internal::Bridge <Object>&> (*this));
	}

	// Query interface
	Internal::Interface* _query_interface (const IDL::String& type_id) const
	{
		if (type_id.empty ())
			return metadata_.primary_interface->proxy;
		else {
			const InterfaceEntry* ie = find_interface (type_id);
			if (ie)
				return ie->proxy;
			else
				return nullptr;
		}
	}

	/// Returns synchronization context for the specific operation.
	virtual Nirvana::Core::SyncContext& get_sync_context (OperationIndex op)
		const noexcept
	{
		assert (op.interface_idx () == 0 && op.operation_idx () != (UShort)ObjectOp::NON_EXISTENT);
		return Nirvana::Core::g_core_free_sync_context;
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
		Internal::IORequest::_ref_type rq = create_request (_make_op_idx ((UShort)ObjectOp::NON_EXISTENT), 3, nullptr);
		rq->invoke ();
		Boolean _ret;
		Internal::Type <Boolean>::unmarshal (rq, _ret);
		rq->unmarshal_end ();
		return _ret;
	}

	virtual Boolean _is_equivalent (Object::_ptr_type other_object) const noexcept
	{
		return &static_cast <const Internal::Bridge <Object>&> (*this) == &other_object;
	}

	virtual Reference* to_reference () noexcept
	{
		return nullptr;
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

	virtual Policy::_ref_type _get_policy (PolicyType policy_type) = 0;
	virtual DomainManagersList _get_domain_managers () = 0;

	Object::_ref_type _set_policy_overrides (const PolicyList& policies, SetOverrideType set_or_add)
	{
		throw NO_IMPLEMENT ();
	}

	Policy::_ref_type _get_client_policy (PolicyType policy_type)
	{
		return _get_policy (policy_type);
	}

	PolicyList _get_policy_overrides (const PolicyTypeSeq& types)
	{
		throw NO_IMPLEMENT ();
	}

	Boolean _validate_connection (PolicyList& inconsistent_policies)
	{
		throw NO_IMPLEMENT ();
	}

	bool has_primary_interface () const noexcept
	{
		return metadata_.primary_interface;
	}

	virtual IDL::String _repository_id ()
	{
		return repository_id ();
	}
	
	Object::_ref_type _get_component ()
	{
		return Object::_nil ();
	}

	// Abstract base implementation

	Object::_ref_type _to_object () noexcept
	{
		return get_proxy ();
	}

	ValueBase::_ref_type _to_value () noexcept
	{
		return nullptr;
	}

	// Misc

	OperationIndex find_operation (Internal::String_in name) const;

	void invoke (OperationIndex op, Internal::IORequest::_ptr_type rq) const noexcept;

	const Internal::Operation& operation_metadata (OperationIndex op) const noexcept
	{
		assert (op.interface_idx () <= metadata_.interfaces.size ());
		if (op.interface_idx () == 0) {
			assert (op.operation_idx () < countof (object_ops_));
			return object_ops_ [op.operation_idx ()];
		} else {
			const InterfaceEntry& itf = metadata_.interfaces [op.interface_idx () - 1];
			return itf.operations.p [op.operation_idx ()];
		}
	}

	static bool is_object_op (OperationIndex op) noexcept
	{
		return op.interface_idx () == 0 && op.operation_idx () != (UShort)ObjectOp::NON_EXISTENT;
	}

	void check_create_request (OperationIndex op, unsigned flags) const;

	// Called from the POA sync context
	virtual ReferenceLocalRef get_local_reference (const PortableServer::Core::POA_Base&);

	virtual ReferenceRef marshal (StreamOut& out) = 0;

	Internal::StringView <Char> primary_interface_id () const noexcept
	{
		if (metadata_.primary_interface)
			return Internal::StringView <Char> (metadata_.primary_interface->iid, metadata_.primary_interface->iid_len);
		else
			return Internal::StringView <Char> (Internal::RepIdOf <Object>::id);
	}

	void check_primary_interface (Internal::String_in primary_iid) const;

	static ProxyManager* cast (Object::_ptr_type obj) noexcept
	{
		return static_cast <ProxyManager*> (static_cast <Internal::Bridge <Object>*> (&obj));
	}

protected:
	ProxyManager (Internal::String_in primary_iid, bool servant_side);
	ProxyManager (const ProxyManager& src);

	~ProxyManager ();

	void set_primary_interface (Internal::String_in primary_iid)
	{
		assert (!metadata_.primary_interface);
		Metadata md (metadata_.heap ());
		build_metadata (md, primary_iid, false);
		metadata_ = std::move (md);
	}

	Internal::IOReference::_ptr_type ior () const noexcept
	{
		return &static_cast <Internal::IOReference&> (static_cast <Internal::Bridge <Internal::IOReference>&> (
			const_cast <ProxyManager&> (*this)));
	}

	struct InterfaceEntry
	{
		const Char* iid;
		size_t iid_len;
		Internal::Interface::_ptr_type implementation;
		Internal::CountedArray <Internal::Operation> operations;
		Internal::ProxyFactory::_ref_type proxy_factory;
		Internal::Interface* proxy;
		Internal::DynamicServant::_ptr_type deleter;

		InterfaceEntry (const InterfaceEntry& src) :
			iid (src.iid),
			iid_len (src.iid_len),
			implementation (nullptr), // implementation is not copied
			operations (src.operations),
			proxy_factory (src.proxy_factory),
			// proxy and deleter are not copied
			proxy (nullptr),
			deleter (nullptr)
		{}

		~InterfaceEntry ()
		{
			if (deleter)
				deleter->delete_object ();
		}
	};

	const InterfaceEntry* find_interface (Internal::String_in iid) const noexcept;

	template <class I>
	void set_servant (Internal::I_ptr <I> servant, size_t offset)
	{
		// Fill implementation pointers
		for (InterfaceEntry* ie = metadata_.interfaces.begin (); ie != metadata_.interfaces.end (); ++ie) {
			assert (!ie->implementation);
			Internal::Interface::_ptr_type impl = servant->_query_interface (ie->iid);
			if (!impl)
				throw OBJ_ADAPTER (); // Implementation not found. TODO: Log
			ie->implementation = offset_ptr (impl, offset);
		}
	}

	void reset_servant () noexcept
	{
		for (InterfaceEntry* ie = metadata_.interfaces.begin (); ie != metadata_.interfaces.end (); ++ie) {
			ie->implementation = nullptr;
		}
	}

	// The implicit object reference operations _non_existent, _is_a, _repository_id and _get_interface
	// may be invoked using DII. No other implicit object reference operations may be invoked via DII.
	// Only _non_existent operation can cause the call to servant.
	// Object operation indexes.
	enum class ObjectOp : UShort
	{
		GET_INTERFACE,
		IS_A,
		NON_EXISTENT,
		REPOSITORY_ID,

		OBJECT_OP_CNT
	};

	OperationIndex _make_op_idx (UShort op_idx) const
	{
		return OperationIndex (0, op_idx);
	}

private:
	struct OperationEntry
	{
		const Char* name;
		size_t name_len;
		OperationIndex idx;
	};

	static InterfaceDef::_ref_type get_interface ();
	static void rq_get_interface (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq);

	Boolean is_a (const IDL::String& type_id) const;
	static void rq_is_a (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq);

	virtual Boolean non_existent ();
	static void rq_non_existent (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq);

	IDL::String repository_id () const
	{
		return primary_interface_id ();
	}

	static void rq_repository_id (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq);

	typedef void (*RqProcInternal) (ProxyManager* servant, Internal::IORequest::_ptr_type call);

	static bool call_request_proc (RqProcInternal proc, ProxyManager* servant, Interface* call);

	template <RqProcInternal proc>
	static bool ObjProcWrapper (Internal::Interface* servant, Internal::Interface* call)
	{
		return call_request_proc (proc, reinterpret_cast <ProxyManager*> ((void*)servant), call);
	}

	struct IEPred;
	struct OEPred;

	void create_proxy (InterfaceEntry& ie, bool servant_side) const;
	void create_proxy (Internal::ProxyFactory::_ptr_type pf,
		const Internal::InterfaceMetadata* metadata, InterfaceEntry& ie, bool servant_side) const;

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

	static Nirvana::Core::Heap& get_heap () noexcept;

	template <class El>
	using Array = Nirvana::Core::Array <El, Nirvana::Core::HeapAllocator>;

	struct Metadata {
		Metadata (Nirvana::Core::Heap& heap) :
			interfaces (heap),
			operations (heap),
			primary_interface (nullptr)
		{}

		Metadata (const Metadata& src, Nirvana::Core::Heap& heap) :
			interfaces (src.interfaces, heap),
			operations (src.operations, heap),
			primary_interface (src.primary_interface)
		{}

		Metadata& operator = (Metadata&& src) = default;

		Nirvana::Core::Heap& heap () const
		{
			return interfaces.get_allocator ().heap ();
		}

		Array <InterfaceEntry> interfaces;
		Array <OperationEntry> operations;
		const InterfaceEntry* primary_interface;
	};

	void build_metadata (Metadata& metadata, Internal::String_in primary_iid, bool servant_side) const;

private:
	// Input parameter metadata for Object::_is_a () operation.
	static const Internal::Parameter is_a_param_;

	// This array contains metadata of the operations:
	// - _get_interface
	// - _is_a
	// - _non_existent
	// - _repository_id
	static const Internal::Operation object_ops_ [(size_t)ObjectOp::OBJECT_OP_CNT];

	Metadata metadata_;
};

}
}

#endif
