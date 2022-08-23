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
#include "ProxyManager.h"
#include "../Binder.h"
#include <algorithm>

using namespace Nirvana;
using namespace CORBA::Internal;
using namespace std;

namespace CORBA {
namespace Core {

struct ProxyManager::IEPred
{
	bool operator () (const InterfaceEntry& lhs, const InterfaceEntry& rhs) const
	{
		return RepId::compare (lhs.iid, lhs.iid_len, rhs.iid, rhs.iid_len) < 0;
	}

	bool operator () (const String& lhs, const InterfaceEntry& rhs) const
	{
		return RepId::compare (rhs.iid, rhs.iid_len, lhs) > 0;
	}

	bool operator () (const InterfaceEntry& lhs, const String& rhs) const
	{
		return RepId::compare (lhs.iid, lhs.iid_len, rhs) < 0;
	}
};

struct ProxyManager::OEPred
{
	bool operator () (const OperationEntry& lhs, const OperationEntry& rhs) const
	{
		return compare (lhs.name, lhs.name_len, rhs.name, rhs.name_len);
	}

	bool operator () (const String& lhs, const OperationEntry& rhs) const
	{
		return compare (lhs.data (), lhs.size (), rhs.name, rhs.name_len);
	}

	bool operator () (const OperationEntry& lhs, const String& rhs) const
	{
		return compare (lhs.name, lhs.name_len, rhs.data (), rhs.size ());
	}

	// Operation names in CORBA are case-insensitive
	static bool less_no_case (Char c1, Char c2)
	{
		return tolower (c1) < tolower (c2);
	}

	static bool compare (const Char* lhs, size_t lhs_len, const Char* rhs, size_t rhs_len);
};

bool ProxyManager::OEPred::compare (const Char* lhs, size_t lhs_len, const Char* rhs, size_t rhs_len)
{
	return lexicographical_compare (lhs, lhs + lhs_len, rhs, rhs + rhs_len, less_no_case);
}

const Parameter ProxyManager::is_a_param_ = { "logical_type_id", Type <String>::type_code };

// Implicit operation names
const Char ProxyManager::op_get_interface_ [] = "_get_interface";
const Char ProxyManager::op_is_a_ []= "_is_a";
const Char ProxyManager::op_non_existent_ [] = "_non_existent";
const Char ProxyManager::op_repository_id_ [] = "_repository_id";

void ProxyManager::check_metadata (const InterfaceMetadata* metadata, String_in primary)
{
	if (!metadata)
		throw OBJ_ADAPTER (); // TODO: Log

	{ // Check interfaces
		size_t itf_cnt = metadata->interfaces.size;
		if (!itf_cnt || itf_cnt > numeric_limits <UShort>::max ())
			throw OBJ_ADAPTER (); // TODO: Log
		const Char* const* itf = metadata->interfaces.p;
		const Char* iid = *itf;
		if (!iid || !RepId::compatible (iid, primary))
			throw OBJ_ADAPTER (); // Primary interface must be first. TODO: Log
		while (--itf_cnt) {
			++itf;
			if (!*itf)
				throw OBJ_ADAPTER (); // TODO: Log
		}
	}

	// Check operations
	for (const Operation* op = metadata->operations.p, *end = op + metadata->operations.size; op != end; ++op) {
		if (!op->name || !op->invoke)
			throw OBJ_ADAPTER (); // TODO: Log
		check_parameters (op->input);
		check_parameters (op->output);
		check_type_code ((op->return_type) ());
	}
}

void ProxyManager::check_parameters (CountedArray <Parameter> parameters)
{
	for (const Parameter* p = parameters.p, *end = p + parameters.size; p != end; ++p) {
		if (!p->name)
			throw OBJ_ADAPTER (); // TODO: Log
		check_type_code ((p->type) ());
	}
}

void ProxyManager::check_type_code (I_ptr <TypeCode> tc)
{
	if (!tc)
		throw OBJ_ADAPTER (); // TODO: Log
}

ProxyManager::ProxyManager (const Bridge <IOReference>::EPV& epv_ior,
	const Bridge <Object>::EPV& epv_obj, const Bridge <AbstractBase>::EPV& epv_ab,
	String_in primary_iid, const OperationsDII& object_ops, void* object_impl) :
	Bridge <IOReference> (epv_ior),
	Bridge <Object> (epv_obj),
	Bridge <AbstractBase> (epv_ab)
{
	ProxyFactory::_ref_type proxy_factory = Nirvana::Core::Binder::bind_interface <ProxyFactory> (primary_iid);

	const InterfaceMetadata* metadata = proxy_factory->metadata ();
	check_metadata (metadata, primary_iid);

	size_t itf_cnt = metadata->interfaces.size;
	interfaces_.allocate (itf_cnt + 1);
	InterfaceEntry* ie = interfaces_.begin ();

	{ // Interface Object
		ie->iid = RepIdOf <Object>::id;
		ie->iid_len = countof (RepIdOf <Object>::id) - 1;
		ie->proxy = &static_cast <Bridge <Object>&> (*this);
		ie->operations.p = object_ops;
		ie->operations.size = size (object_ops);
		ie->implementation = reinterpret_cast <Interface*> (object_impl);
		++ie;
	}

	// Proxy interface version can be different
	const Char* proxy_primary_iid;

	{ // Fill interface table
		const Char* const* itf = metadata->interfaces.p;
		proxy_primary_iid = *itf;

		do {
			const Char* iid = *itf;
			ie->iid = iid;
			ie->iid_len = strlen (iid);
			++itf;
		} while (interfaces_.end () != ++ie);
	}

	sort (interfaces_.begin (), interfaces_.end (), IEPred ());

	// Check that all interfaces are unique
	if (!is_unique (interfaces_.begin (), interfaces_.end (), IEPred ()))
		throw OBJ_ADAPTER (); // TODO: Log

	// Create base proxies
	ie = interfaces_.begin ();
	InterfaceEntry* primary = nullptr;
	do {
		const Char* iid = ie->iid;
		if (iid == proxy_primary_iid)
			primary = ie;
		else if (iid == RepIdOf <Object>::id)
			object_itf_idx_ = (UShort)(ie - interfaces_.begin ());
		else
			create_proxy (*ie);
	} while (interfaces_.end () != ++ie);

	// Create primary proxy
	assert (primary);
	create_proxy (proxy_factory, *primary);
	primary->operations = metadata->operations;
	primary_interface_ = primary;

	// Total count of operations
	size_t op_cnt = 0;
	ie = interfaces_.begin ();
	do {
		op_cnt += ie->operations.size;
	} while (interfaces_.end () != ++ie);

	// Fill operation table
	operations_.allocate (op_cnt);
	OperationEntry* op = operations_.begin ();
	ie = interfaces_.begin ();
	do {
		IOReference::OperationIndex idx ((UShort)(ie - interfaces_.begin ()), 0);
		for (const Operation* p = ie->operations.p, *end = p + ie->operations.size; p != end; ++p) {
			const Char* name = p->name;
			op->name = name;
			op->name_len = strlen (name);
			op->idx = idx;
			++idx.operation_idx ();
			++op;
		}
	} while (interfaces_.end () != ++ie);

	sort (operations_.begin (), operations_.end (), OEPred ());
	
	// Check name uniqueness

	if (!is_unique (operations_.begin (), operations_.end (), OEPred ()))
		throw OBJ_ADAPTER (); // TODO: Log
}

ProxyManager::~ProxyManager ()
{
	for (InterfaceEntry* ie = interfaces_.begin (); ie != interfaces_.end (); ++ie) {
		if (ie->deleter)
			ie->deleter->delete_object ();
	}
}

void ProxyManager::create_proxy (ProxyFactory::_ptr_type pf, InterfaceEntry& ie)
{
	Interface* deleter;
	Interface* proxy = pf->create_proxy (ior (), (UShort)(&ie - interfaces_.begin ()), deleter);
	if (!proxy || !deleter)
		throw MARSHAL ();
	ie.deleter = DynamicServant::_check (deleter);
	try {
		ie.proxy = Interface::_check (proxy, ie.iid);
	} catch (...) {
		ie.deleter->delete_object ();
		throw;
	}
}

void ProxyManager::create_proxy (InterfaceEntry& ie)
{
	if (!ie.proxy) {
		StringBase <Char> iid (ie.iid);
		ProxyFactory::_ref_type pf = Nirvana::Core::Binder::bind_interface <ProxyFactory> (iid);
		const InterfaceMetadata* metadata = pf->metadata ();
		check_metadata (metadata, iid);
		const Char* const* base = metadata->interfaces.p;
		const Char* const* base_end = base + metadata->interfaces.size;
		ie.iid = *base; // Base proxy may have greater minor number.
		++base;
		for (; base != base_end; ++base) {
			InterfaceEntry* base_ie = const_cast <InterfaceEntry*> (find_interface (*base));
			if (!base_ie)
				throw OBJ_ADAPTER (); // Base is not listed in the primary interface base list. TODO: Log
			create_proxy (*base_ie);
		}
		create_proxy (pf, ie);
		ie.operations = metadata->operations;
	}
}

const ProxyManager::InterfaceEntry* ProxyManager::find_interface (String_in iid) const
{
	const String& siid = static_cast <const String&> (iid);
	const InterfaceEntry* pf = lower_bound (interfaces_.begin (), interfaces_.end (), siid, IEPred ());
	if (pf != interfaces_.end () && RepId::compatible (pf->iid, pf->iid_len, siid))
		return pf;
	return nullptr;
}

IOReference::OperationIndex ProxyManager::find_operation (const IDL::String& name) const
{
	const OperationEntry* pf = lower_bound (operations_.begin (), operations_.end (), name, OEPred ());
	if (pf != operations_.end () && !OEPred () (name, *pf))
		return pf->idx;
	throw BAD_OPERATION (MAKE_OMG_MINOR (2));
}

void ProxyManager::serve_object_request (ObjectOp op, Internal::IORequest::_ptr_type rq)
{
	switch (op) {
		case ObjectOp::GET_INTERFACE: {
			rq->unmarshal_end ();
			InterfaceDef::_ref_type ref = get_interface ();
			Type <InterfaceDef>::marshal_out (ref, rq);
		} break;

		case ObjectOp::IS_A: {
			IDL::String type_id;
			Type <IDL::String>::unmarshal (rq, type_id);
			rq->unmarshal_end ();
			Boolean ret = is_a (type_id);
			Type <Boolean>::marshal_out (ret, rq);
		} break;

		case ObjectOp::REPOSITORY_ID: {
			rq->unmarshal_end ();
			IDL::String ret = repository_id ();
			Type <IDL::String>::marshal_out (ret, rq);
		} break;

		default:
			assert (false);
	}
	rq->success ();
}

InterfaceDef::_ref_type ProxyManager::get_interface ()
{
	return InterfaceDef::_nil (); // TODO: Implement.
}

}
}
