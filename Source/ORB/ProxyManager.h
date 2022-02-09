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
#include "../CoreObject.h"
#include <CORBA/Proxy/InterfaceMetadata.h>
#include <CORBA/Proxy/ProxyFactory.h>

namespace CORBA {
namespace Internal {
namespace Core {

/// \brief Base for all proxies.
class ProxyManager :
	public ::Nirvana::Core::CoreObject,
	public Bridge <IOReference>,
	public Bridge <Object>
{
public:
	Bridge <Object>* _get_object (String_in iid)
	{
		if (RepId::check (Object::repository_id_, iid) != RepId::COMPATIBLE)
			::Nirvana::throw_INV_OBJREF ();
		return this;
	}

	Object::_ptr_type get_proxy () NIRVANA_NOEXCEPT
	{
		return object ();
	}

	Object::_ptr_type object () NIRVANA_NOEXCEPT
	{
		return &static_cast <Object&> (static_cast <Bridge <Object>&> (*this));
	}

	// Abstract base
	Interface* _query_interface (const String& type_id) const
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

	I_ref <InterfaceDef> _get_interface () const
	{
		SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, nullptr);
		return InterfaceDef::_nil (); // TODO: Implement.
		SYNC_END ();
	}

	Boolean _is_a (const String& type_id) const
	{
		String tmp (type_id);
		SYNC_BEGIN (Nirvana::Core::g_core_free_sync_context, nullptr);
		return find_interface (tmp) != nullptr;
		SYNC_END ();
	}

	Boolean _non_existent ()
	{
		IORequest::_ref_type rq = ior ()->create_request (_make_op_idx (OBJ_OP_NON_EXISTENT));
		rq->invoke ();
		Boolean _ret;
		Type <Boolean>::unmarshal (rq, _ret);
		return _ret;
	}

	Boolean _is_equivalent (Object::_ptr_type other_object) const
	{
		return &static_cast <const Bridge <Object>&> (*this) == &other_object;
	}

	ULong _hash (ULong maximum) const
	{
		return (ULong)((uintptr_t)this 
			- ::Nirvana::Core::Port::Memory::query (nullptr, ::Nirvana::Memory::QueryParam::ALLOCATION_SPACE_BEGIN)
			) % maximum;
	}

	// TODO: More Object operations shall be here...

protected:
	ProxyManager (const Bridge <IOReference>::EPV& epv_ior, const Bridge <Object>::EPV& epv_obj,
		String_in primary_iid, const Operation object_ops [3], void* object_impl);

	~ProxyManager ();

	IOReference::_ptr_type ior ()
	{
		return &static_cast <IOReference&> (static_cast <Bridge <IOReference>&> (*this));
	}

	struct InterfaceEntry
	{
		const Char* iid;
		size_t iid_len;
		Interface* proxy;
		DynamicServant::_ptr_type deleter;
		Interface::_ptr_type implementation;
		CountedArray <Operation> operations;
	};

	struct OperationEntry
	{
		const Char* name;
		size_t name_len;
		IOReference::OperationIndex idx;
	};

	::Nirvana::Core::Array <InterfaceEntry, ::Nirvana::Core::CoreAllocator>& interfaces ()
	{
		return interfaces_;
	}

	const InterfaceEntry* find_interface (String_in iid) const;
	const OperationEntry* find_operation (String_in name) const;

	UShort object_itf_idx () const
	{
		return object_itf_idx_;
	}

	IOReference::OperationIndex _make_op_idx (UShort op_idx) const
	{
		return IOReference::OperationIndex (object_itf_idx_, op_idx);
	}

	// Object operation indexes
	enum
	{
		OBJ_OP_GET_INTERFACE,
		OBJ_OP_IS_A,
		OBJ_OP_NON_EXISTENT
	};

	// Metadata details

	// Output param structure for Boolean returning operations.
	struct BooleanRet
	{
		Type <Boolean>::ABI _ret;
	};

	struct get_interface_out
	{
		Interface* _ret;
	};

	struct is_a_in
	{
		ABI <String> logical_type_id;
	};

	// Input parameter metadata for `is_a` operation.
	static const Parameter is_a_param_;

	// Implicit operation names
	static const Char op_get_interface_ [];
	static const Char op_is_a_ [];
	static const Char op_non_existent_ [];

private:
	struct IEPred;
	struct OEPred;

	void create_proxy (InterfaceEntry& ie);
	void create_proxy (ProxyFactory::_ptr_type pf, InterfaceEntry& ie);

	static void check_metadata (const InterfaceMetadata* metadata, String_in primary);
	static void check_parameters (CountedArray <Parameter> parameters);
	static void check_type_code (I_ptr <TypeCode> tc);
	
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
	::Nirvana::Core::Array <InterfaceEntry, ::Nirvana::Core::CoreAllocator> interfaces_;
	::Nirvana::Core::Array <OperationEntry, ::Nirvana::Core::CoreAllocator> operations_;

	const InterfaceEntry* primary_interface_;
	UShort object_itf_idx_;
};

}
}
}

#endif
