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

	Poller (ProxyManager& proxy, Internal::IOReference::OperationIndex op);

	virtual Internal::Interface* _query_valuetype (CORBA::Internal::String_in id) noexcept override;

	Object::_ref_type operation_target () const noexcept
	{
		return proxy_->get_proxy ();
	}

	IDL::String operation_name () const
	{
		return proxy_->operation_metadata (op_).name;
	}

	Messaging::ReplyHandler::_ref_type associated_handler () const noexcept
	{
		return associated_handler_;
	}

	void associated_handler (Messaging::ReplyHandler::_ptr_type handler) noexcept
	{
		associated_handler_ = handler;
	}

	bool is_from_poller () const noexcept
	{
		return is_from_poller_;
	}

	Internal::IORequest::_ref_type get_reply (uint32_t timeout, Internal::IOReference::OperationIndex op)
	{
		if (op.interface_idx () != op_.interface_idx () || op.operation_idx () != op_.operation_idx ()) {
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
		SYNC_BEGIN (sync_domain (), nullptr)
		if (!reply_) {
			is_from_poller_ = true;
			throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (5));
		}
		is_from_poller_ = false;
		return std::move (reply_);
		SYNC_END ()
	}

private:
	virtual void on_complete (Internal::IORequest::_ptr_type reply) override;

private:
	struct ValueEntry
	{
		const Char* iid;
		size_t iid_len;
		Internal::Interface::_ptr_type value;
		Internal::DynamicServant::_ptr_type deleter;
	};

	Nirvana::Core::Array <ValueEntry, Nirvana::Core::UserAllocator> values_;

	servant_reference <ProxyManager> proxy_;
	Messaging::ReplyHandler::_ref_type associated_handler_;
	Internal::IORequest::_ref_type reply_;
	const Internal::IOReference::OperationIndex op_;
	bool is_from_poller_;
};

inline
Poller::Poller (ProxyManager& proxy, Internal::IOReference::OperationIndex op) :
	proxy_ (&proxy),
	op_ (op),
	is_from_poller_ (false)
{

}

}
}

#endif
