/// \file ProxyManager.h
#ifndef NIRVANA_ORB_CORE_PROXYMANAGER_H_
#define NIRVANA_ORB_CORE_PROXYMANAGER_H_

#include "../Synchronized.h"
#include "../Array.h"
#include <CORBA/Proxy/Proxy.h>

namespace CORBA {
namespace Nirvana {
namespace Core {

/// \brief Base for all proxies.
class ProxyManager :
	public Bridge <IOReference>,
	public Bridge <Object>
{
public:
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

	InterfaceDef_var _get_interface () const
	{
		::Nirvana::Core::Synchronized sync (::Nirvana::Core::SynchronizationContext::free_sync_context ());
		return InterfaceDef::_nil (); // TODO: Implement.
	}

	Boolean _is_a (const String& type_id) const
	{
		String tmp (type_id);
		::Nirvana::Core::Synchronized sync (::Nirvana::Core::SynchronizationContext::free_sync_context ());
		return find_interface (tmp) != nullptr;
	}

	Boolean _non_existent ()
	{
		BooleanRet out;
		Marshal_var m;
		ior ()->call (OperationIndex{ object_itf_idx (), OBJ_OP_NON_EXISTENT }, nullptr, 0, m, &out, sizeof (out));
		Boolean _ret;
		_unmarshal (out._ret, Unmarshal::_nil (), _ret);
		return _ret;
	}

	Boolean _is_equivalent (Object_ptr other_object) const
	{
		return &static_cast <const Bridge <Object>&> (*this) == &other_object;
	}

	ULong _hash (ULong maximum) const
	{
		return (ULong)(uintptr_t)this % maximum;
	}

	// TODO: More Object operations shall be here...

protected:
	ProxyManager (const Bridge <IOReference>::EPV& epv_ior, const Bridge <Object>::EPV& epv_obj,
		String_in primary_iid, const Operation object_ops [3], void* object_impl);

	~ProxyManager ();

	IOReference_ptr ior ()
	{
		return &static_cast <IOReference&> (static_cast <Bridge <IOReference>&> (*this));
	}

	struct InterfaceEntry
	{
		const Char* iid;
		size_t iid_len;
		Interface* proxy;
		DynamicServant_ptr deleter;
		Interface* implementation;
		CountedArray <Operation> operations;
	};

	struct OperationEntry
	{
		const Char* name;
		size_t name_len;
		OperationIndex idx;
	};

	::Nirvana::Core::Array <InterfaceEntry>& interfaces ()
	{
		return interfaces_;
	}

	const InterfaceEntry* find_interface (String_in iid) const;
	const OperationEntry* find_operation (String_in name) const;

	UShort object_itf_idx () const
	{
		return object_itf_idx_;
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
		ABI_boolean _ret;
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

	static void check_metadata (const InterfaceMetadata* metadata, String_in primary);
	static void check_parameters (CountedArray <Parameter> parameters);
	static void check_type_code (const ::Nirvana::ImportInterfaceT <TypeCode>& tc);
	
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
	::Nirvana::Core::Array <InterfaceEntry> interfaces_;
	::Nirvana::Core::Array <OperationEntry> operations_;

	const InterfaceEntry* primary_interface_;
	UShort object_itf_idx_;
};

}
}
}

#endif
