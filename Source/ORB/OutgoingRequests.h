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
#ifndef NIRVANA_ORB_CORE_OUTGOINGREQUESTS_H_
#define NIRVANA_ORB_CORE_OUTGOINGREQUESTS_H_
#pragma once

#include "../SkipList.h"
#include "RequestOut.h"
#include <type_traits>

namespace CORBA {
namespace Core {

/// Outgoing requests manager
class OutgoingRequests
{
	static const unsigned SKIP_LIST_LEVELS = 10; // TODO: config.h

public:
	/// Called on system startup
	static void initialize ()
	{
		map_.construct ();
	}

	/// Called on system shutdown
	static void terminate ()
	{
		map_.destruct ();
	}

	static void new_request (RequestOut& rq, RequestOut::IdPolicy id_policy);
	static void new_request_oneway (RequestOut& rq, RequestOut::IdPolicy id_policy);
	static servant_reference <RequestOut> remove_request (RequestOut::RequestId request_id) noexcept;
	static void receive_reply (unsigned GIOP_minor, servant_reference <StreamIn>&& stream);

	static void receive_reply_immediate (uint32_t request_id, servant_reference <StreamIn>&& stream) noexcept
	{
		servant_reference <RequestOut> rq = remove_request (request_id);
		if (rq) {
			try {
				receive_reply_internal (*rq, 1, (RequestId)request_id, 0, IOP::ServiceContextList (), std::move (stream));
			} catch (const SystemException& ex) {
				on_reply_exception (*rq, ex, GIOP::ReplyStatusType::NO_EXCEPTION);
			}
		}
	}

	static void set_system_exception (uint32_t request_id, const SystemException& ex) noexcept
	{
		set_system_exception (request_id, ex.__code (), ex.minor (), ex.completed ());
	}

	static void set_system_exception (uint32_t request_id, SystemException::Code code,
		uint32_t minor, CompletionStatus completed) noexcept;

private:
	typedef RequestOut::RequestId RequestId;

	static void receive_reply_internal (RequestOut& rq, unsigned GIOP_minor, RequestId request_id,
		uint32_t status, const IOP::ServiceContextList& context1,
		servant_reference <StreamIn>&& stream);

	static void on_reply_exception (RequestOut& rq, const SystemException& ex,
		GIOP::ReplyStatusType status) noexcept;

	struct RequestVal
	{
		RequestVal (RequestId id, RequestOut& rq) noexcept :
			request_id (id),
			request (&rq)
		{}

		RequestVal (RequestId id) noexcept :
			request_id (id)
		{}

		bool operator < (const RequestVal& rhs) const noexcept
		{
			return request_id < rhs.request_id;
		}

		RequestId request_id;
		servant_reference <RequestOut> request;
	};

	// The request map
	typedef Nirvana::Core::SkipList <RequestVal, SKIP_LIST_LEVELS> RequestMap;

	static Nirvana::Core::StaticallyAllocated <RequestMap> map_;
};

}
}

#endif
