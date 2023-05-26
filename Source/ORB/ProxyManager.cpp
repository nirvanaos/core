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
using namespace Nirvana::Core;

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
	bool operator () (const OperationEntry& lhs, const OperationEntry& rhs) const NIRVANA_NOEXCEPT
	{
		return less (lhs.name, lhs.name_len, rhs.name, rhs.name_len);
	}

	bool operator () (String_in lhs, const OperationEntry& rhs) const NIRVANA_NOEXCEPT
	{
		return less (lhs.data (), lhs.size (), rhs.name, rhs.name_len);
	}

	bool operator () (const OperationEntry& lhs, String_in rhs) const NIRVANA_NOEXCEPT
	{
		return less (lhs.name, lhs.name_len, rhs.data (), rhs.size ());
	}

	static bool less (const Char* lhs, size_t lhs_len, const Char* rhs, size_t rhs_len);
};

bool ProxyManager::OEPred::less (const Char* lhs, size_t lhs_len, const Char* rhs, size_t rhs_len)
{
	return std::lexicographical_compare (lhs, lhs + lhs_len, rhs, rhs + rhs_len);
}

void ProxyManager::check_metadata (const InterfaceMetadata* metadata, String_in primary)
{
	if (!metadata)
		throw OBJ_ADAPTER (); // TODO: Log

	{ // Check interfaces
		size_t itf_cnt = metadata->interfaces.size;
		if (!itf_cnt || itf_cnt > std::numeric_limits <UShort>::max ())
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
		if (op->return_type)
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

ProxyManager::ProxyManager (String_in primary_iid, bool servant_side) :
	primary_interface_ (nullptr)
{
	if (!primary_iid.empty ()) {
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

		std::sort (interfaces_.begin (), interfaces_.end (), IEPred ());

		// Check that all interfaces are unique
		if (!is_unique (interfaces_.begin (), interfaces_.end (), IEPred ()))
			throw OBJ_ADAPTER (); // TODO: Log

		// Create base proxies
		InterfaceEntry* primary = nullptr;
		ie = interfaces_.begin ();
		do {
			const Char* iid = ie->iid;
			if (iid == proxy_primary_iid)
				primary = ie;
			else
				create_proxy (*ie, servant_side);
		} while (interfaces_.end () != ++ie);

		// Create primary proxy
		assert (primary);
		create_proxy (proxy_factory, metadata, *primary, servant_side);
		primary->operations = metadata->operations;
		primary_interface_ = primary;
	}

	// Total count of operations
	size_t op_cnt = countof (object_ops_);
	for (const InterfaceEntry* ie = interfaces_.begin (); ie != interfaces_.end (); ++ie) {
		op_cnt += ie->operations.size;
	}

	// Fill operation table
	operations_.allocate (op_cnt);
	OperationEntry* op = operations_.begin ();

	// Object operations
	OperationIndex idx (0, 0);
	for (const Operation* p = object_ops_, *e = std::end (object_ops_); p != e; ++p) {
		const Char* name = p->name;
		op->name = name;
		op->name_len = strlen (name);
		op->idx = idx;
		++idx.operation_idx ();
		++op;
	}

	// Interface operations
	for (const InterfaceEntry* ie = interfaces_.begin (); ie != interfaces_.end (); ++ie) {
		++idx.interface_idx ();
		idx.operation_idx (0);
		for (const Operation* p = ie->operations.p, *end = p + ie->operations.size; p != end; ++p) {
			const Char* name = p->name;
			op->name = name;
			op->name_len = strlen (name);
			op->idx = idx;
			++idx.operation_idx ();
			++op;
		}
	}

	std::sort (operations_.begin (), operations_.end (), OEPred ());

	// Check name uniqueness

	if (!is_unique (operations_.begin (), operations_.end (), OEPred ()))
		throw OBJ_ADAPTER (); // TODO: Log
}

ProxyManager::ProxyManager (const ProxyManager& src) :
	primary_interface_ (src.primary_interface_),
	interfaces_ (src.interfaces_),
	operations_ (src.operations_)
{
	for (InterfaceEntry* ie = interfaces_.begin (); ie != interfaces_.end (); ++ie) {
		create_proxy (*ie, false);
	}
}

ProxyManager::~ProxyManager ()
{}

void ProxyManager::check_primary_interface (String_in primary_iid) const
{
	// Empty primary IID does not change anything
	if (!(primary_iid.empty () || RepId::compatible (RepIdOf <Object>::id, primary_iid))) {
		const InterfaceEntry* pi = primary_interface_;
		if (pi && (pi->iid_len != primary_iid.size () ||
			!std::equal (pi->iid, pi->iid + pi->iid_len, primary_iid.data ())))
			throw BAD_PARAM ();
	}
}

void ProxyManager::create_proxy (ProxyFactory::_ptr_type pf, const InterfaceMetadata* metadata,
	InterfaceEntry& ie, bool servant_side)
{
	if (servant_side && metadata->flags & InterfaceMetadata::FLAG_NO_PROXY)
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

void ProxyManager::create_proxy (InterfaceEntry& ie, bool servant_side)
{
	if (!ie.proxy) {
		const InterfaceMetadata* metadata;
		if (!ie.proxy_factory) {
			CORBA::Internal::StringView <Char> iid (ie.iid);
			ie.proxy_factory = Nirvana::Core::Binder::bind_interface <ProxyFactory> (iid);
			metadata = ie.proxy_factory->metadata ();
			check_metadata (metadata, iid);
		} else
			metadata = ie.proxy_factory->metadata ();
		const Char* const* base = metadata->interfaces.p;
		const Char* const* base_end = base + metadata->interfaces.size;
		ie.iid = *base; // Base proxy may have greater minor number.
		++base;
		for (; base != base_end; ++base) {
			InterfaceEntry* base_ie = const_cast <InterfaceEntry*> (find_interface (*base));
			if (!base_ie)
				throw OBJ_ADAPTER (); // Base is not listed in the primary interface base list. TODO: Log
			create_proxy (*base_ie, servant_side);
		}
		ie.operations = metadata->operations;
		create_proxy (ie.proxy_factory, metadata, ie, servant_side);
	}
}

const ProxyManager::InterfaceEntry* ProxyManager::find_interface (String_in iid) const
	NIRVANA_NOEXCEPT
{
	const String& siid = static_cast <const String&> (iid);
	const InterfaceEntry* pf = std::lower_bound (interfaces_.begin (), interfaces_.end (), siid, IEPred ());
	if (pf != interfaces_.end () && RepId::compatible (pf->iid, pf->iid_len, siid))
		return pf;
	return nullptr;
}

IOReference::OperationIndex ProxyManager::find_operation (String_in name) const
{
	const OperationEntry* pf = std::lower_bound (operations_.begin (), operations_.end (), name, OEPred ());
	if (pf != operations_.end () && !OEPred () (name, *pf))
		return pf->idx;
	throw BAD_OPERATION (MAKE_OMG_MINOR (2));
}

IORequest::_ref_type ProxyManager::create_request (OperationIndex op, unsigned flags)
{
	assert (is_object_op (op));
	unsigned response_flags = flags & 3;
	if (response_flags == 2)
		throw INV_FLAG ();

	// Do not create new memory context for the trivial object operations.
	// New memory context may be created only for _get_interface () because it can create a new object.
	MemContext* memory = (op.operation_idx () == (UShort)ObjectOp::GET_INTERFACE)
		? nullptr : &MemContext::current ();

	if (flags & IOReference::REQUEST_ASYNC)
		return make_pseudo <RequestLocalImpl <RequestLocalAsync> > (std::ref (*this), op,
			memory, response_flags);
	else
		return make_pseudo <RequestLocalImpl <RequestLocal> > (std::ref (*this), op,
			memory, response_flags);
}

void ProxyManager::check_create_request (OperationIndex op, unsigned flags) const
{
	if ((flags & 3) == 2)
		throw INV_FLAG ();
	size_t itf = op.interface_idx ();
	size_t op_cnt;
	if (0 == itf)
		op_cnt = (size_t)ObjectOp::OBJECT_OP_CNT;
	else if (itf > interfaces_.size ())
		throw BAD_PARAM ();
	else
		op_cnt = interfaces_ [itf - 1].operations.size;
	if (op.operation_idx () >= op_cnt)
		throw BAD_PARAM ();
}

void ProxyManager::invoke (OperationIndex op, IORequest::_ptr_type rq) const NIRVANA_NOEXCEPT
{
	try {
		try {
			size_t itf_idx = op.interface_idx ();
			size_t op_idx = op.operation_idx ();
			Interface* implementation;
			const Operation* op_metadata;
			if (0 == itf_idx) { // Object operation
				assert (op_idx < countof (object_ops_));
				implementation = reinterpret_cast <Interface*> ((void*)this);
				op_metadata = object_ops_ + op_idx;
			} else {
				--itf_idx;
				const InterfaceEntry& ie = interfaces_ [itf_idx];
				implementation = &ie.implementation;
				if (!implementation)
					throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));
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

Boolean ProxyManager::non_existent ()
{
	assert (false); // Must not be called in this class.
	return false;
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

ReferenceLocalRef ProxyManager::get_local_reference (const PortableServer::Core::POA_Base&)
{
	return nullptr;
}

const Parameter ProxyManager::is_a_param_ = { "logical_type_id", Type <String>::type_code };

const Operation ProxyManager::object_ops_ [(size_t)ObjectOp::OBJECT_OP_CNT] = {
	{ "_get_interface", {0, 0}, {0, 0}, {0, 0}, {0, 0}, Type <InterfaceDef>::type_code, ObjProcWrapper <rq_get_interface>, Operation::FLAG_OUT_CPLX },
	{ "_is_a", {&is_a_param_, 1}, {0, 0}, {0, 0}, {0, 0}, Type <Boolean>::type_code, ObjProcWrapper <rq_is_a>, 0 },
	{ "_non_existent", {0, 0}, {0, 0}, {0, 0}, {0, 0}, Type <Boolean>::type_code, ObjProcWrapper <rq_non_existent>, 0 },
	{ "_repository_id", {0, 0}, {0, 0}, {0, 0}, {0, 0}, Type <String>::type_code, ObjProcWrapper <rq_repository_id>, 0 }
};

}
}
