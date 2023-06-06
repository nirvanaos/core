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

#include "DomainAddress.h"
#include "RequestGIOP.h"
#include "RequestInPOA.h"
#include "../ExecDomain.h"
#include "../Timer.h"

namespace CORBA {
namespace Core {

/// Unique id of an incoming request.
struct RequestKey : DomainAddress
{
	RequestKey (const DomainAddress& addr, uint32_t rq_id) :
		DomainAddress (addr),
		request_id (rq_id)
	{}

	RequestKey (const DomainAddress& addr) :
		DomainAddress (addr)
	{}

	bool operator < (const RequestKey& rhs) const NIRVANA_NOEXCEPT
	{
		if (request_id < rhs.request_id)
			return true;
		else if (request_id > rhs.request_id)
			return false;
		else
			return DomainAddress::operator < (rhs);
	}

	uint32_t request_id;
};

/// Implements server-side IORequest for GIOP.
class NIRVANA_NOVTABLE RequestIn :
	public RequestGIOP,
	public RequestInPOA
{
protected:
	template <class T>
	friend class Nirvana::Core::Ref;
	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	virtual Nirvana::Core::MemContext* memory () const NIRVANA_NOEXCEPT override;

public:
	void initialize (Nirvana::Core::Ref <StreamIn>&& in);

	const RequestKey& key () const NIRVANA_NOEXCEPT
	{
		return key_;
	}

	/// \returns The request id.
	uint32_t request_id () const NIRVANA_NOEXCEPT
	{
		return key_.request_id;
	}

	/// \returns Service context sorted in the ascending order.
	const IOP::ServiceContextList& service_context () const NIRVANA_NOEXCEPT
	{
		return service_context_;
	}

	virtual const IOP::ObjectKey& object_key () const NIRVANA_NOEXCEPT override
	{
		return object_key_;
	}

	virtual Internal::StringView <Char> operation () const NIRVANA_NOEXCEPT override
	{
		return operation_;
	}

	virtual void serve (const ServantProxyBase& proxy) override;

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

	void inserted_to_map (void* iter) NIRVANA_NOEXCEPT
	{
		map_iterator_ = iter;
	}

	/// Finalizes the request.
	/// 
	/// \returns `true` if the reply must be sent.
	bool finalize () NIRVANA_NOEXCEPT;

protected:
	RequestIn (const DomainAddress& client, unsigned GIOP_minor);
	~RequestIn ();

	/// Output stream factory.
	/// Must be overridden in a derived class.
	/// 
	/// \returns The output stream reference.
	virtual Nirvana::Core::Ref <StreamOut> create_output () = 0;

	void post_send_success () NIRVANA_NOEXCEPT;

private:
	void get_object_key (const IOP::TaggedProfile& profile);

	// Caller operations throw BAD_OPERATION or just return `false`
	virtual void invoke () override;
	virtual bool get_exception (Any& e) override;

	virtual void unmarshal_end () override;
	virtual bool marshal_op () override;

	void switch_to_reply (GIOP::ReplyStatusType status = GIOP::ReplyStatusType::NO_EXCEPTION);

	void delayed_release () NIRVANA_NOEXCEPT;

protected:
	RequestKey key_;
	IOP::ServiceContextList service_context_;

private:
	class DelayedRelease : public Nirvana::Core::Runnable
	{
	public:
		DelayedRelease (RequestIn& request) :
			request_ (request)
		{}

	private:
		virtual void run () override;

	private:
		RequestIn& request_;
	};

	class DelayedReleaseTimer : public Nirvana::Core::Timer
	{
	public:
		DelayedReleaseTimer (RequestIn& request) :
			request_ (request)
		{}

	private:
		virtual void signal () noexcept override;

	private:
		RequestIn& request_;
	};

	IOP::ObjectKey object_key_;
	IDL::String operation_;
	void* map_iterator_;

	/// SyncDomain of the target object.
	Nirvana::Core::SyncDomain* sync_domain_;

	size_t reply_status_offset_;
	size_t reply_header_end_;
	// We should not use StaticallyAllocated here because the constructed_ value will be undefined.
	std::aligned_storage <sizeof (DelayedReleaseTimer), alignof (DelayedReleaseTimer)>::type delayed_release_timer_;
	std::atomic <bool> cancelled_;
	bool has_context_;
};

}
}

#endif
