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
#include "../pch.h"
#include "ProxyManager.h"
#include "../Binder.h"
#include "RequestLocal.h"
#include "Poller.h"
#include <algorithm>

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
using namespace Internal;
namespace Core {

const Parameter ProxyManager::is_a_param_ = { "logical_type_id", Type <String>::type_code };

const Operation ProxyManager::object_ops_ [(size_t)ObjectOp::OBJECT_OP_CNT] = {
	{ "_interface", {0, 0}, {0, 0}, {0, 0}, {0, 0}, Type <InterfaceDef>::type_code,
		call <rq_get_interface>, Operation::FLAG_OUT_CPLX },
	{ "_is_a", {&is_a_param_, 1}, {0, 0}, {0, 0}, {0, 0}, Type <Boolean>::type_code,
		call <rq_is_a>, 0 },
	{ "_non_existent", {0, 0}, {0, 0}, {0, 0}, {0, 0}, Type <Boolean>::type_code,
		call <rq_non_existent>, 0 },
	{ "_domain_managers", {0, 0}, {0, 0}, {0, 0}, {0, 0}, Type <Boolean>::type_code,
		call <rq_domain_managers>, Operation::FLAG_OUT_CPLX },
	{ "_repository_id", {0, 0}, {0, 0}, {0, 0}, {0, 0}, Type <String>::type_code,
		call <rq_repository_id>, 0 },
	{ "_component", {0, 0}, {0, 0}, {0, 0}, {0, 0}, Type <String>::type_code,
		call <rq_component>, Operation::FLAG_OUT_CPLX }
};

struct ProxyManager::OEPred
{
	bool operator () (const OperationEntry& lhs, const OperationEntry& rhs) const noexcept
	{
		return less (lhs.name, lhs.name_len, rhs.name, rhs.name_len);
	}

	bool operator () (String_in lhs, const OperationEntry& rhs) const noexcept
	{
		return less (lhs.data (), lhs.size (), rhs.name, rhs.name_len);
	}

	bool operator () (const OperationEntry& lhs, String_in rhs) const noexcept
	{
		return less (lhs.name, lhs.name_len, rhs.data (), rhs.size ());
	}

	static bool less (const Char* lhs, size_t lhs_len, const Char* rhs, size_t rhs_len);
};

bool ProxyManager::OEPred::less (const Char* lhs, size_t lhs_len, const Char* rhs, size_t rhs_len)
{
	return std::lexicographical_compare (lhs, lhs + lhs_len, rhs, rhs + rhs_len);
}

Heap& ProxyManager::get_heap () noexcept
{
	SyncDomain* sd = SyncContext::current ().sync_domain ();
	if (sd)
		return sd->mem_context ().heap ();
	else
		return Heap::shared_heap ();
}

void ProxyManager::check_metadata (const InterfaceMetadata* metadata, String_in primary,
	bool servant_side)
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
	unsigned local = metadata->flags & InterfaceMetadata::FLAG_LOCAL;
	bool need_invoke = servant_side && (metadata->flags & InterfaceMetadata::FLAG_LOCAL_STATELESS) != InterfaceMetadata::FLAG_LOCAL_STATELESS;
	for (const Operation* op = metadata->operations.p, *end = op + metadata->operations.size; op != end; ++op) {
		if ((!local && !op->name) || (need_invoke && !op->invoke))
			throw OBJ_ADAPTER (); // TODO: Log
		if (!local) {
			check_parameters (op->input);
			check_parameters (op->output);
			if (op->return_type)
				check_type_code ((op->return_type) ());
			for (const GetTypeCode* ue = op->user_exceptions.p, *end = ue + op->user_exceptions.size; ue != end; ++ue) {
				TypeCode::_ptr_type tc = (*ue) ();
				check_type_code (tc);
				if (tc->kind () != TCKind::tk_except)
					throw OBJ_ADAPTER (); // TODO: Log
			}
		}
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
	metadata_ (get_heap ())
{
	build_metadata (metadata_, primary_iid, servant_side);
}

void ProxyManager::build_metadata (Metadata& md, String_in primary_iid, bool servant_side) const
{
	unsigned local_object = 0;
	if (!primary_iid.empty ()) {
		ProxyFactory::_ref_type proxy_factory = Nirvana::Core::Binder::bind_interface <ProxyFactory> (primary_iid);

		const InterfaceMetadata* metadata = proxy_factory->metadata ();
		check_metadata (metadata, primary_iid, servant_side);

		local_object = metadata->flags & InterfaceMetadata::FLAG_LOCAL;

		// Proxy interface version can be different
		const Char* proxy_primary_iid = metadata->interfaces.p [0];

		// Fill interface table
		size_t itf_cnt = metadata->interfaces.size;
		md.interfaces.allocate (itf_cnt);

		InterfaceEntry* ie = md.interfaces.begin ();
		{
			const Char* const* itf = metadata->interfaces.p;
			do {
				const Char* iid = *itf;
				ie->iid = iid;
				ie->iid_len = strlen (iid);
				++itf;
			} while (md.interfaces.end () != ++ie);
		}

		std::sort (md.interfaces.begin (), md.interfaces.end ());

		// Check that all interfaces are unique
		if (!is_ascending (md.interfaces.begin (), md.interfaces.end (), std::less <InterfaceId> ()))
			throw OBJ_ADAPTER (); // TODO: Log

		// Create base proxies
		InterfaceEntry* primary = nullptr;
		ie = md.interfaces.begin ();
		do {
			const Char* iid = ie->iid;
			if (iid == proxy_primary_iid)
				primary = ie;
			else
				create_proxy (*ie, servant_side);
		} while (md.interfaces.end () != ++ie);

		// Create primary proxy
		assert (primary);
		primary->interface_metadata = metadata;
		primary->proxy_factory = std::move (proxy_factory);
		create_proxy (*primary);
		md.primary_interface = primary;
	}

	if (!local_object) {

		// Total count of operations
		size_t op_cnt = countof (object_ops_);
		for (const InterfaceEntry* ie = md.interfaces.begin (); ie != md.interfaces.end (); ++ie) {
			op_cnt += ie->operations ().size;
		}

		// Fill operation table
		md.operations.allocate (op_cnt);
		OperationEntry* op = md.operations.begin ();

		// Object operations
		UShort itf_idx = 0;
		OperationIndex idx = 0;
		for (const Operation* p = object_ops_, *e = std::end (object_ops_); p != e; ++p) {
			const Char* name = p->name;
			op->name = name;
			op->name_len = strlen (name);
			op->idx = idx;
			++idx;
			++op;
		}

		// Interface operations
		for (const InterfaceEntry* ie = md.interfaces.begin (); ie != md.interfaces.end (); ++ie) {
			idx = make_op_idx (++itf_idx, 0);
			const auto& operations = ie->operations ();
			for (const Operation* p = operations.p, *end = p + operations.size; p != end; ++p) {
				const Char* name = p->name;
				op->name = name;
				op->name_len = strlen (name);
				op->idx = idx;
				++idx;
				++op;
			}
		}

		std::sort (md.operations.begin (), md.operations.end (), OEPred ());

		// Check name uniqueness

		if (!is_ascending (md.operations.begin (), md.operations.end (), OEPred ()))
			throw OBJ_ADAPTER (); // TODO: Log
	}
}

ProxyManager::ProxyManager (const ProxyManager& src) :
	metadata_ (src.metadata_, get_heap ())
{
	for (InterfaceEntry* ie = metadata_.interfaces.begin (); ie != metadata_.interfaces.end (); ++ie) {
		create_proxy (*ie, false);
	}
}

ProxyManager::~ProxyManager ()
{}

void ProxyManager::check_primary_interface (String_in primary_iid) const
{
	// Empty primary IID does not change anything
	if (!(primary_iid.empty () || RepId::compatible (RepIdOf <Object>::id, primary_iid))) {
		const InterfaceEntry* pi = metadata_.primary_interface;
		if (pi && (pi->iid_len != primary_iid.size () ||
			!std::equal (pi->iid, pi->iid + pi->iid_len, primary_iid.data ())))
			throw BAD_PARAM ();
	}
}

void ProxyManager::create_proxy (InterfaceEntry& ie) const
{
	Interface::_ref_type holder;
	Interface* proxy = ie.proxy_factory->create_proxy (get_proxy (), get_abstract_base (), ior (),
		(UShort)(&ie - metadata_.interfaces.begin () + 1),
		ie.implementation, holder);
	if (!proxy || !holder)
		throw MARSHAL ();
	ie.proxy = Interface::_check (proxy, ie.iid);
	ie.holder = std::move (holder);
}

void ProxyManager::create_proxy (InterfaceEntry& ie, bool servant_side) const
{
	if (!ie.proxy) {
		const InterfaceMetadata* metadata;
		if (!ie.proxy_factory) {
			CORBA::Internal::StringView <Char> iid (ie.iid);
			ie.proxy_factory = Nirvana::Core::Binder::bind_interface <ProxyFactory> (iid);
			metadata = ie.proxy_factory->metadata ();
			check_metadata (metadata, iid, servant_side);
			ie.interface_metadata = metadata;
		} else
			metadata = &ie.metadata ();
		const Char* const* base = metadata->interfaces.p;
		const Char* const* base_end = base + metadata->interfaces.size;
		ie.iid = *base; // Base proxy may have greater minor number.
		++base;
		for (; base != base_end; ++base) {
			InterfaceEntry* base_ie = const_cast <InterfaceEntry*> (find_interface (*base));
			if (!base_ie)
				throw OBJ_ADAPTER (); // Base is not listed in the primary interface base list. TODO: Log
			create_proxy (*base_ie, servant_side);
			// Proxy may be already created in nested call
			if (ie.proxy)
				return;
		}
		create_proxy (ie);
	}
}

const ProxyManager::InterfaceEntry* ProxyManager::find_interface (String_in iid) const
	noexcept
{
	const InterfaceEntry* pf = std::lower_bound (metadata_.interfaces.begin (),
		metadata_.interfaces.end (), iid);
	if (pf != metadata_.interfaces.end () && RepId::compatible (pf->iid, pf->iid_len, iid))
		return pf;
	return nullptr;
}

OperationIndex ProxyManager::find_operation (String_in name) const
{
	const OperationEntry* pf = std::lower_bound (metadata_.operations.begin (),
		metadata_.operations.end (), name, OEPred ());
	if (pf != metadata_.operations.end () && !OEPred () (name, *pf))
		return pf->idx;
	throw BAD_OPERATION (MAKE_OMG_MINOR (2));
}

IORequest::_ref_type ProxyManager::create_request (OperationIndex op, unsigned flags,	CallbackRef&& callback)
{
	assert (is_local_object_op (op));
	if (flags == 2 || flags > 3)
		throw INV_FLAG ();

	// Do not create new memory context for the trivial object operations.
	// New memory context may be created only for _get_interface () because it can create a new object.
	Heap* memory = (operation_idx (op) == (UShort)ObjectOp::GET_INTERFACE)
		? nullptr : &MemContext::current ().heap ();

	if (callback) {
		if (!(flags & IORequest::RESPONSE_EXPECTED))
			throw BAD_PARAM ();
		return make_pseudo <RequestLocalImpl <RequestLocalAsync> > (std::ref (*this), op,
			memory, flags, std::move (callback));
	} else if (flags & IORequest::RESPONSE_EXPECTED) {
		return make_pseudo <RequestLocalImpl <RequestLocal> > (std::ref (*this), op,
			memory, flags);
	} else {
		return make_pseudo <RequestLocalImpl <RequestLocalOneway> > (std::ref (*this), op,
			memory, flags);
	}
}

void ProxyManager::check_create_request (OperationIndex op, unsigned flags) const
{
	if (flags == 2 || flags > 3)
		throw INV_FLAG ();
	size_t itf = interface_idx (op);
	size_t op_cnt;
	if (0 == itf)
		op_cnt = (size_t)ObjectOp::OBJECT_OP_CNT;
	else if (itf > metadata_.interfaces.size ())
		throw BAD_PARAM ();
	else
		op_cnt = metadata_.interfaces [itf - 1].operations ().size;
	if (operation_idx (op) >= op_cnt)
		throw BAD_PARAM ();
}

void ProxyManager::invoke (OperationIndex op, IORequest::_ptr_type rq) const noexcept
{
	try {
		try {
			size_t itf_idx = interface_idx (op);
			size_t op_idx = operation_idx (op);
			Interface* implementation;
			const Operation* op_metadata;
			if (0 == itf_idx) { // Object operation
				assert (op_idx < countof (object_ops_));
				implementation = reinterpret_cast <Interface*> ((void*)this);
				op_metadata = object_ops_ + op_idx;
			} else {
				--itf_idx;
				const InterfaceEntry& ie = metadata_.interfaces [itf_idx];
				implementation = ie.implementation;
				if (!implementation)
					throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));
				assert (op_idx < ie.operations ().size);
				op_metadata = ie.operations ().p + op_idx;
			}

			if (!(*(op_metadata->invoke)) (implementation, &rq))
				throw UNKNOWN ();

		} catch (Exception& e) {
			Any any = RqHelper::exception2any (std::move (e));
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
	NIRVANA_UNREACHABLE_CODE (); // Must not be called in this class.
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

void ProxyManager::rq_domain_managers (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq)
{
	DomainManagersList ret = servant->_get_domain_managers ();
	Type <DomainManagersList>::marshal_out (ret, _rq);
}

void ProxyManager::rq_component (ProxyManager* servant, CORBA::Internal::IORequest::_ptr_type _rq)
{
	Object::_ref_type ret = servant->_get_component ();
	Type <Object>::marshal_out (ret, _rq);
}

ReferenceLocalRef ProxyManager::get_local_reference (const PortableServer::Core::POA_Base&)
{
	return nullptr;
}

CORBA::Pollable::_ref_type ProxyManager::create_poller (OperationIndex op)
{
	return make_reference <Poller> (std::ref (*this), op);
}

const Operation& ProxyManager::operation_metadata (OperationIndex op) const noexcept
{
	size_t itf_idx = interface_idx (op);
	assert (itf_idx <= metadata_.interfaces.size ());
	if (itf_idx == 0) {
		assert (operation_idx (op) < countof (object_ops_));
		return object_ops_ [operation_idx (op)];
	} else {
		const InterfaceEntry& itf = metadata_.interfaces [itf_idx - 1];
		return itf.operations ().p [operation_idx (op)];
	}
}

OperationIndex ProxyManager::find_handler_operation (OperationIndex op,
	Object::_ptr_type handler) const
{
	assert (handler);
	size_t itf_idx = interface_idx (op);
	assert (itf_idx && itf_idx <= metadata_.interfaces.size ());
	const InterfaceEntry& itf = metadata_.interfaces [itf_idx - 1];
	assert (itf.metadata ().handler_id);
	ProxyManager* handler_proxy = cast (handler);
	const InterfaceEntry* handler_itf = handler_proxy->find_interface (itf.metadata ().handler_id);
	if (!handler_itf)
		throw BAD_PARAM ();
	return make_op_idx ((UShort)(handler_itf - handler_proxy->interfaces ().begin () + 1),
		operation_idx (op) * 2);
}

}
}
