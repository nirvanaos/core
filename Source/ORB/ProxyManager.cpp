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
#include "RequestLocal.h"
#include <algorithm>

using namespace Nirvana;
using namespace std;

namespace CORBA {
using namespace Internal;
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

void ProxyManager::check_type_code (TypeCode::_ptr_type tc)
{
	if (!tc || !RepId::compatible (tc->_epv ().header.interface_id, RepIdOf <TypeCode>::id))
		throw OBJ_ADAPTER (); // TODO: Log
}

ProxyManager::ProxyManager (String_in primary_iid)
{
	ProxyFactory::_ref_type proxy_factory = Nirvana::Core::Binder::bind_interface <ProxyFactory> (primary_iid);

	const InterfaceMetadata* metadata = proxy_factory->metadata ();
	check_metadata (metadata, primary_iid);

	// Proxy interface version can be different
	const Char* proxy_primary_iid = metadata->interfaces.p [0];

	// Fill interface table
	size_t itf_cnt = metadata->interfaces.size;
	interfaces_.allocate (itf_cnt);
	InterfaceEntry* ie = interfaces_.begin ();
	{
		const Char* const* itf = metadata->interfaces.p;
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
		else
			create_proxy (*ie);
	} while (interfaces_.end () != ++ie);

	// Create primary proxy
	assert (primary);
	create_proxy (proxy_factory, metadata, *primary);
	primary->operations = metadata->operations;
	primary_interface_ = primary;

	// Total count of operations
	size_t op_cnt = size (object_ops_);
	ie = interfaces_.begin ();
	do {
		op_cnt += ie->operations.size;
	} while (interfaces_.end () != ++ie);

	// Fill operation table
	operations_.allocate (op_cnt);
	OperationEntry* op = operations_.begin ();

	// Object operations
	IOReference::OperationIndex idx (0, 0);
	for (const Operation* p = object_ops_, *e = end (object_ops_); p != e; ++p) {
		const Char* name = p->name;
		op->name = name;
		op->name_len = strlen (name);
		op->idx = idx;
		++idx.operation_idx ();
		++op;
	}

	// Interface operations
	ie = interfaces_.begin ();
	do {
		++idx.interface_idx ();
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

void ProxyManager::create_proxy (ProxyFactory::_ptr_type pf, const InterfaceMetadata* metadata, InterfaceEntry& ie)
{
	if (metadata->flags & InterfaceMetadata::FLAG_NO_PROXY)
		ie.proxy = &ie.implementation;
	else {
		Interface* deleter;
		Interface* proxy = pf->create_proxy (ior (), (UShort)(&ie - interfaces_.begin () + 1), deleter);
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
		create_proxy (pf, metadata, ie);
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

IORequest::_ref_type ProxyManager::create_request (IOReference::OperationIndex op, UShort flags)
{
	assert (is_object_op (op));
	UShort response_flags = flags & 3;
	if (response_flags == 2)
		throw INV_FLAG ();
	if (flags & IOReference::REQUEST_ASYNC)
		return make_pseudo <RequestLocalImpl <RequestLocalAsync> > (std::ref (*this), op,
			nullptr, response_flags);
	else
		return make_pseudo <RequestLocalImpl <RequestLocal> > (std::ref (*this), op,
			nullptr, response_flags);
}

void ProxyManager::invoke (IOReference::OperationIndex op, IORequest::_ptr_type rq) NIRVANA_NOEXCEPT
{
	try {
		try {
			size_t itf_idx = op.interface_idx ();
			size_t op_idx = op.operation_idx ();
			Interface* implementation;
			const Operation* op_metadata;
			if (0 == itf_idx) { // Object operation
				assert (op_idx < std::size (object_ops_));
				implementation = reinterpret_cast <Interface*> ((void*)this);
				op_metadata = object_ops_ + op_idx;
			} else {
				--itf_idx;
				const InterfaceEntry& ie = interfaces () [itf_idx];
				implementation = &ie.implementation;
				if (!implementation)
					throw NO_IMPLEMENT ();
				assert (op_idx < ie.operations.size);
				op_metadata = ie.operations.p + op_idx;
			}

			if (!(*(op_metadata->invoke)) (implementation, &rq))
				throw UNKNOWN ();

		} catch (Exception& e) {
			Any any;
			any <<= std::move (e);
			rq->set_exception (any);
		}
	} catch (...) {
		Any any;
		try {
			any <<= UNKNOWN ();
			rq->set_exception (any);
		} catch (...) {
		}
	}
}

InterfaceDef::_ref_type ProxyManager::get_interface ()
{
	return InterfaceDef::_nil (); // TODO: Implement.
}

void ProxyManager::rq_get_interface (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq)
{
	_rq->unmarshal_end ();
	InterfaceDef::_ref_type ret = servant->get_interface ();
	Type <InterfaceDef>::marshal_out (ret, _rq);
}

Boolean ProxyManager::is_a (const IDL::String& type_id) const
{
	if (find_interface (type_id) != nullptr)
		return true;
	else
		return Internal::RepId::compatible (Internal::RepIdOf <Object>::id, type_id);
}

void ProxyManager::rq_is_a (ProxyManager* servant, IORequest::_ptr_type _rq)
{
	IDL::String iid;
	Type <IDL::String>::unmarshal (_rq, iid);
	_rq->unmarshal_end ();
	Boolean ret = servant->is_a (iid);
	Type <Boolean>::marshal_out (ret, _rq);
}

void ProxyManager::rq_non_existent (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq)
{
	_rq->unmarshal_end ();
	Boolean ret = servant->non_existent ();
	Type <Boolean>::marshal_out (ret, _rq);
}

void ProxyManager::rq_repository_id (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq)
{
	_rq->unmarshal_end ();
	IDL::String ret = servant->repository_id ();
	Type <IDL::String>::marshal_out (ret, _rq);
}

bool ProxyManager::call_request_proc (RqProcInternal proc, ProxyManager* servant, Interface* call)
{
	IORequest::_ptr_type rq = IORequest::_nil ();
	try {
		rq = IORequest::_check (call);
		proc (servant, rq);
		rq->success ();
	} catch (Exception& e) {
		if (!rq)
			return false;

		Any any;
		any <<= std::move (e);
		rq->set_exception (any);
	}
	return true;
}

const Parameter ProxyManager::is_a_param_ = { "logical_type_id", Type <String>::type_code };

const OperationsDII ProxyManager::object_ops_ = {
	{ "_get_interface", {0, 0}, {0, 0}, Type <InterfaceDef>::type_code, ObjProcWrapper <rq_get_interface> },
	{ "_is_a", {&is_a_param_, 1}, {0, 0}, Type <Boolean>::type_code, ObjProcWrapper <rq_is_a> },
	{ "_non_existent", {0, 0}, {0, 0}, Type <Boolean>::type_code, ObjProcWrapper <rq_non_existent> },
	{ "_repository_id", {0, 0}, {0, 0}, Type <String>::type_code, ObjProcWrapper <rq_repository_id> }
};

}
}
