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
#ifndef NIRVANA_ORB_CORE_REQUESTLOCALPOA_H_
#define NIRVANA_ORB_CORE_REQUESTLOCALPOA_H_
#pragma once

#include "RequestLocal.h"
#include "RequestInPOA.h"
#include "ObjectKey.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE RequestLocalPOA :
	public RequestLocalBase,
	public RequestInPOA
{
public:
	virtual const PortableServer::Core::ObjectKey& object_key () const NIRVANA_NOEXCEPT override
	{
		return *object_key_;
	}

protected:
	RequestLocalPOA (ProxyManager& proxy, Internal::IOReference::OperationIndex op,
		PortableServer::Core::ObjectKeyRef&& obj_key, UShort response_flags) :
		RequestLocalBase (nullptr, response_flags),
		object_key_ (std::move (obj_key))
	{
		callee_memory_ = caller_memory_;
		operation_ = proxy.operation_metadata (op).name;
	}

	virtual void set_exception (Any& e) override
	{
		RequestLocalBase::set_exception (e);
	}

	virtual void invoke () override;
	virtual void serve_request (ProxyObject& proxy, Internal::IOReference::OperationIndex op,
		Nirvana::Core::MemContext* memory) override;

	virtual bool is_cancelled () const NIRVANA_NOEXCEPT override
	{
		return RequestLocalBase::is_cancelled ();
	}

	// Override RequestInPOA::_add_ref ()
	void _add_ref () NIRVANA_NOEXCEPT override
	{
		RequestLocalBase::_add_ref ();
	}

private:
	PortableServer::Core::ObjectKeyRef object_key_;
};

class NIRVANA_NOVTABLE RequestLocalAsyncPOA :
	public RequestLocalPOA
{
protected:
	RequestLocalAsyncPOA (ProxyManager& proxy, Internal::IOReference::OperationIndex op,
		PortableServer::Core::ObjectKeyRef&& obj_key, UShort response_flags) :
		RequestLocalPOA (proxy, op, std::move (obj_key), response_flags)
	{}

	virtual void invoke () override;
	virtual void serve_request (ProxyObject& proxy, Internal::IOReference::OperationIndex op,
		Nirvana::Core::MemContext* memory) override;
	virtual void cancel () NIRVANA_NOEXCEPT override;

private:
	Nirvana::Core::CoreRef <Nirvana::Core::ExecDomain> exec_domain_;
};

}
}

#endif
