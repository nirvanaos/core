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

#include "RequestGIOP.h"
#include "RequestInPOA.h"
#include "IncomingRequests.h"
#include "../UserObject.h"
#include "../ExecDomain.h"
#include "GIOP.h"

namespace CORBA {
namespace Core {

/// Implements server-side IORequest for GIOP.
class NIRVANA_NOVTABLE RequestIn :
	public RequestGIOP,
	public RequestInPOA
{
protected:
	template <class T>
	friend class Nirvana::Core::CoreRef;
	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	virtual Nirvana::Core::MemContext* memory () const NIRVANA_NOEXCEPT override;

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

	/// \returns Service context.
	const IOP::ServiceContextList& service_context () const NIRVANA_NOEXCEPT
	{
		return service_context_;
	}

	virtual const PortableServer::Core::ObjectKey& object_key () const NIRVANA_NOEXCEPT override;
	virtual Internal::StringView <Char> operation () const NIRVANA_NOEXCEPT override;

	/// Cancel the request.
	/// Called from the IncomingRequests class.
	/// Request is already removed from map on this call.
	virtual void cancel () NIRVANA_NOEXCEPT override;
	virtual bool is_cancelled () const NIRVANA_NOEXCEPT override;

	/// Return exception to caller.
	/// Operation has move semantics so \p e may be cleared.
	/// Must be overridden in the derived class to send reply.
	virtual void set_exception (Any& e) override = 0;

	/// Marks request as successful.
	/// Must be overridden in the derived class to send reply.
	virtual void success () override = 0;

	void inserted_to_map (IncomingRequests::MapIter it) NIRVANA_NOEXCEPT
	{
		map_iterator_ = it;
	}

	/// Finalizes the request.
	/// 
	/// \returns `true` if the reply must be sent.
	bool finalize ();

protected:
	RequestIn (const DomainAddress& client, unsigned GIOP_minor, Nirvana::Core::CoreRef <StreamIn>&& in);
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
	virtual void serve_request (ProxyObject& proxy, Internal::IOReference::OperationIndex op,
		Nirvana::Core::MemContext* memory) override;

protected:
	RequestKey key_;
	IOP::ServiceContextList service_context_;

private:
	PortableServer::Core::ObjectKey object_key_;
	IDL::String operation_;
	IncomingRequests::MapIter map_iterator_;

	/// ExecDomain pointer if request is in cancellable state, otherwise `nullptr`.
	/// While request in map, exec_domain_ is not `nullptr`.
	/// For the oneway requests, exec_domain_ is always `nullptr`.
	Nirvana::Core::ExecDomain* exec_domain_;

	/// SyncDomain of the target object.
	Nirvana::Core::SyncDomain* sync_domain_;

	size_t reply_status_offset_;
	size_t reply_header_end_;
	std::atomic <bool> cancelled_;
};

}
}

#endif
