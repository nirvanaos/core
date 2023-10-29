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
#ifndef NIRVANA_ORB_CORE_POLLER_H_
#define NIRVANA_ORB_CORE_POLLER_H_
#pragma once

#include "Pollable.h"
#include "ProxyManager.h"
#include <CORBA/Messaging_s.h>

namespace CORBA {
namespace Core {

class ProxyManager;

class Poller :
	public Pollable,
	public Internal::ServantTraits <Poller>,
	public Internal::ValueImpl <Poller, Messaging::Poller>,
	public Internal::LifeCycleRefCnt <Poller>
{
public:
	using Internal::ServantTraits <Poller>::_wide_val;
	using Internal::ServantTraits <Poller>::_implementation;
	using Internal::LifeCycleRefCnt <Poller>::__duplicate;
	using Internal::LifeCycleRefCnt <Poller>::__release;
	using Internal::LifeCycleRefCnt <Poller>::_duplicate;
	using Internal::LifeCycleRefCnt <Poller>::_release;

	Poller (ProxyManager& proxy, Internal::OperationIndex op);
	Poller (const Poller& src);

	virtual Internal::Interface* _query_valuetype (const IDL::String& id) noexcept override;

	Object::_ptr_type operation_target () const noexcept
	{
		return proxy_->get_proxy ();
	}

	const IDL::String& operation_name () const
	{
		return operation_name_;
	}

	Messaging::ReplyHandler::_ptr_type associated_handler () const noexcept
	{
		return associated_handler_;
	}

	void associated_handler (Messaging::ReplyHandler::_ptr_type handler) noexcept
	{
		if (handler)
			op_handler_ = proxy_->find_handler_operation (op_, handler);
		associated_handler_ = handler;
	}

	bool is_from_poller () const noexcept
	{
		return is_from_poller_;
	}

	Internal::IORequest::_ref_type get_reply (uint32_t timeout, Internal::OperationIndex op)
	{
		if (op != op_) {
			is_from_poller_ = true;
			throw WrongTransaction ();
		}
		if (!is_ready (timeout)) {
			is_from_poller_ = true;
			if (timeout)
				throw TIMEOUT (MAKE_OMG_MINOR (1));
			else
				throw NO_RESPONSE (MAKE_OMG_MINOR (1));
		}
		Internal::IORequest::_ref_type ret;
		SYNC_BEGIN (sync_domain (), nullptr)
		if (!reply_) {
			is_from_poller_ = true;
			throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (5));
		}
		is_from_poller_ = false;
		ret = std::move (reply_);
		SYNC_END ();
		return ret;
	}

private:
	virtual void on_complete (Internal::IORequest::_ptr_type reply) override;

	struct ValueEntry : public InterfaceId
	{
		Internal::Interface::_ptr_type value;
		Internal::Interface::_ref_type holder;

		ValueEntry (const ValueEntry& src) :
			InterfaceId (src),
			value (nullptr),
			holder ()
		{}

		ValueEntry (ValueEntry&& src) = default;

		~ValueEntry ()
		{}

		ValueEntry& operator = (ValueEntry&& src) = default;
	};

	ValueEntry* find_value (Internal::String_in iid);
	void create_value (ValueEntry& ve, const ProxyManager::InterfaceEntry& ie);

private:
	servant_reference <ProxyManager> proxy_;
	Internal::StringView <Char> operation_name_;
	Messaging::ReplyHandler::_ref_type associated_handler_;
	Internal::IORequest::_ref_type reply_;
	Nirvana::Core::Array <ValueEntry, Nirvana::Core::UserAllocator> values_;
	const ValueEntry* primary_;
	const Internal::OperationIndex op_;
	Internal::OperationIndex op_handler_;
	bool is_from_poller_;
};

inline
Poller::Poller (ProxyManager& proxy, Internal::OperationIndex op) :
	proxy_ (&proxy),
	operation_name_ (proxy.operation_metadata (op).name),
	primary_ (nullptr),
	op_ (op),
	op_handler_ (0),
	is_from_poller_ (false)
{
	const ProxyManager::InterfaceEntry* primary_interface = proxy.primary_interface ();
	assert (primary_interface && primary_interface->metadata ().poller_id);
	if (!(primary_interface && primary_interface->metadata ().poller_id))
		throw NO_IMPLEMENT ();

	size_t val_cnt = 0;
	for (auto pi = proxy.interfaces ().begin (), end = proxy.interfaces ().end (); pi != end; ++pi) {
		if (pi->metadata ().poller_id)
			++val_cnt;
	}

	assert (val_cnt);
	val_cnt += 2;

	values_.allocate (val_cnt);
	auto pv = values_.begin ();
	pv->iid = Internal::RepIdOf <CORBA::Pollable>::id;
	pv->iid_len = std::size (Internal::RepIdOf <CORBA::Pollable>::id) - 1;
	pv->value = CORBA::Pollable::_ptr_type (this);
	++pv;
	pv->iid = Internal::RepIdOf <Messaging::Poller>::id;
	pv->iid_len = std::size (Internal::RepIdOf <Messaging::Poller>::id) - 1;
	pv->value = Messaging::Poller::_ptr_type (this);
	++pv;

	const Char* primary_id = nullptr;
	for (auto pi = proxy.interfaces ().begin (), end = proxy.interfaces ().end (); pi != end; ++pi) {
		const Char* id = pi->metadata ().poller_id;
		if (id) {
			pv->iid = id;
			pv->iid_len = strlen (id);
			if (primary_interface == pi)
				primary_id = id;
			++pv;
		}
	}

	std::sort (values_.begin (), values_.end ());

	// Check that all interfaces are unique
	if (!is_ascending (values_.begin (), values_.end (), std::less <InterfaceId> ()))
		throw OBJ_ADAPTER (); // TODO: Log

	primary_ = find_value (primary_id);

	// Create values
	create_value (const_cast <ValueEntry&> (*primary_), *primary_interface);
}

inline
Poller::Poller (const Poller& src) :
	proxy_ (src.proxy_),
	operation_name_ (src.operation_name_),
	associated_handler_ (src.associated_handler_),
	values_ (src.values_),
	primary_ (values_.begin () + (src.primary_ - src.values_.begin ())),
	op_ (src.op_),
	op_handler_ (src.op_handler_),
	is_from_poller_ (false)
{
	find_value (Internal::RepIdOf <CORBA::Pollable>::id)->value = CORBA::Pollable::_ptr_type (this);
	find_value (Internal::RepIdOf <Messaging::Poller>::id)->value = Messaging::Poller::_ptr_type (this);

	// Create values
	create_value (const_cast <ValueEntry&> (*primary_), *proxy_->primary_interface ());
}

}
}

#endif
