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
#include <CORBA/Messaging_s.h>

namespace Messaging {
namespace Core {

class Poller :
	public CORBA::Core::Pollable,
	public CORBA::Internal::ValueTraits <Poller>,
	public CORBA::Internal::ServantTraits <Poller>,
	public CORBA::Internal::ValueImpl <Poller, Messaging::Poller>,
	public CORBA::Internal::LifeCycleRefCnt <Poller>
{
public:
	using CORBA::Internal::ServantTraits <Poller>::_wide_val;
	using CORBA::Internal::ServantTraits <Poller>::_implementation;
	using CORBA::Internal::LifeCycleRefCnt <Poller>::__duplicate;
	using CORBA::Internal::LifeCycleRefCnt <Poller>::__release;
	using CORBA::Internal::LifeCycleRefCnt <Poller>::_duplicate;
	using CORBA::Internal::LifeCycleRefCnt <Poller>::_release;

	Interface* _query_valuetype (CORBA::Internal::String_in id) noexcept
	{
		return CORBA::Internal::FindInterface <Messaging::Poller, CORBA::Pollable>::find (*this, id);
	}

	CORBA::Object::_ref_type operation_target () const noexcept
	{
		return operation_target_;
	}

	IDL::String operation_name () const
	{
		return operation_name_;
	}

	ReplyHandler::_ref_type associated_handler () const noexcept
	{
		return associated_handler_;
	}

	void associated_handler (ReplyHandler::_ptr_type handler) noexcept
	{
		associated_handler_ = handler;
	}

	bool is_from_poller () const noexcept
	{
		return is_from_poller_;
	}

	void set_reply (CORBA::Internal::IORequest::_ptr_type reply) noexcept
	{
		request_ = reply;
	}

	CORBA::Internal::IORequest::_ref_type get_reply () noexcept
	{
		return std::move (request_);
	}

protected:
	Poller (CORBA::Object::_ptr_type target, const char* operation_name) :
		operation_target_ (target),
		operation_name_ (operation_name),
		is_from_poller_ (false)
	{}

private:
	CORBA::Object::_ref_type operation_target_;
	const char* operation_name_;
	ReplyHandler::_ref_type associated_handler_;
	CORBA::Internal::IORequest::_ref_type request_;
	bool is_from_poller_;
};

}
}

#endif
