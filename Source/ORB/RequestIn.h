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
#ifndef NIRVANA_ORB_CORE_REQUESTIN_H_
#define NIRVANA_ORB_CORE_REQUESTIN_H_
#pragma once

#include "Request.h"
#include "ClientAddress.h"
#include "../UserObject.h"
#include "../ExecDomain.h"
#include "GIOP.h"

namespace CORBA {
namespace Core {

/// Unique id of an incoming request.
struct RequestKey : ClientAddress
{
	RequestKey (const ClientAddress& addr, uint32_t rq_id) :
		ClientAddress (addr),
		request_id (rq_id)
	{}

	RequestKey (const ClientAddress& addr) :
		ClientAddress (addr)
	{}

	bool operator < (const RequestKey& rhs) const NIRVANA_NOEXCEPT
	{
		if (request_id < rhs.request_id)
			return true;
		else if (request_id > rhs.request_id)
			return false;
		else
			return ClientAddress::operator < (rhs);
	}

	uint32_t request_id;
};

/// Implements server-side IORequest for GIOP.
class NIRVANA_NOVTABLE RequestIn :
	public Request,
	public Nirvana::Core::UserObject
{
public:
	const RequestKey& key () const NIRVANA_NOEXCEPT
	{
		return key_;
	}

	/// \returns The request id.
	uint32_t request_id () const NIRVANA_NOEXCEPT
	{
		return key_.request_id;
	}

	/// \returns Target object key.
	const IOP::ObjectKey& object_key () const NIRVANA_NOEXCEPT
	{
		return object_key_;
	}

	/// \returns Operation name.
	const IDL::String& operation () const NIRVANA_NOEXCEPT
	{
		return operation_;
	}

	/// \returns Service context.
	const IOP::ServiceContextList& service_context () const NIRVANA_NOEXCEPT
	{
		return service_context_;
	}

	/// \returns  Response flags.
	unsigned response_flags () const NIRVANA_NOEXCEPT
	{
		return response_flags_;
	}

	/// Cancel the request.
	/// Called from the IncomingRequests class.
	/// Request is already removed from map on this call.
	virtual void cancel () override;

	/// Return exception to caller.
	/// Operation has move semantics so \p e may be cleared.
	/// Must be overridden in the derived class to send reply.
	virtual void set_exception (Any& e) override = 0;

	/// Marks request as successful.
	/// Must be overridden in the derived class to send reply.
	virtual void success () override = 0;

	/// Finalizes the request.
	/// 
	/// \returns `true` if the reply must be sent.
	bool finalize ();

	Nirvana::Core::ExecDomain* exec_domain () const NIRVANA_NOEXCEPT
	{
		return exec_domain_;
	}

	void exec_domain (Nirvana::Core::ExecDomain* ed) NIRVANA_NOEXCEPT
	{
		exec_domain_ = ed;
	}

protected:
	RequestIn (const ClientAddress& client, unsigned GIOP_minor, Nirvana::Core::CoreRef <StreamIn>&& in);

	~RequestIn ();

	/// Output stream factory.
	/// Must be overridden in a derived class.
	/// 
	/// \param [out] GIOP_minor GIOP version minor number.
	/// \returns The output stream reference.
	virtual Nirvana::Core::CoreRef <StreamOut> create_output () = 0;

private:
	void get_object_key (const IOP::TaggedProfile& profile);

	// Caller operations throw BAD_OPERATION or just return `false`
	virtual void invoke () override;
	virtual bool is_exception () const NIRVANA_NOEXCEPT override;
	virtual bool completed () const NIRVANA_NOEXCEPT override;
	virtual bool wait (uint64_t timeout) override;

	virtual void unmarshal_end () override;
	virtual bool marshal_op () override;

	void switch_to_reply (GIOP::ReplyStatusType status = GIOP::ReplyStatusType::NO_EXCEPTION);

protected:
	RequestKey key_;
	IOP::ObjectKey object_key_;
	IDL::String operation_;
	IOP::ServiceContextList service_context_;
	unsigned response_flags_;
	unsigned GIOP_minor_;

private:
	Nirvana::Core::ExecDomain* exec_domain_;
	size_t reply_status_offset_;
	size_t reply_header_end_;
};

}
}

#endif
