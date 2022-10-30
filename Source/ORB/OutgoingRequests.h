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
		singleton_.construct ();
	}

	/// Called on system shutdown
	static void terminate ()
	{
		singleton_.destruct ();
	}

	enum class IdPolicy
	{
		ANY,
		EVEN,
		ODD
	};

	static uint32_t new_request (RequestOut& rq, IdPolicy id_policy);

	static uint32_t new_request_oneway (IdPolicy id_policy)
	{
		return singleton_->new_request_oneway_internal (id_policy);
	}

	static Nirvana::Core::CoreRef <RequestOut> remove_request (uint32_t request_id) NIRVANA_NOEXCEPT;

	static void receive_reply (unsigned GIOP_minor, Nirvana::Core::CoreRef <StreamIn>&& stream);

	static void set_system_exception (uint32_t request_id, const Char* rep_id,
		uint32_t minor, CompletionStatus completed) NIRVANA_NOEXCEPT;

private:
	// Request id generator.
#if ATOMIC_LONG_LOCK_FREE
	typedef long IdGenType;
#elif ATOMIC_INT_LOCK_FREE
	typedef int IdGenType;
#elif ATOMIC_LLONG_LOCK_FREE
	typedef long long IdGenType;
#elif ATOMIC_SHORT_LOCK_FREE
	typedef short IdGenType;
#else
#error Platform does not meet the minimal atomic requirements.
#endif

	// IdGen may have different sizes. We need 32-bit max, if possible.
	typedef std::conditional_t <(sizeof (IdGenType) <= 4), IdGenType, uint32_t> RequestId;

	RequestId get_new_id (IdPolicy id_policy) NIRVANA_NOEXCEPT;

	struct RequestVal
	{
		RequestVal (RequestId id, RequestOut& rq) NIRVANA_NOEXCEPT :
			request_id (id),
			request (&rq)
		{}

		RequestVal (RequestId id) NIRVANA_NOEXCEPT :
			request_id (id)
		{}

		bool operator < (const RequestVal& rhs) const NIRVANA_NOEXCEPT
		{
			return request_id < rhs.request_id;
		}

		RequestId request_id;
		Nirvana::Core::CoreRef <RequestOut> request;
	};

	// The request map
	typedef Nirvana::Core::SkipList <RequestVal, SKIP_LIST_LEVELS> RequestMap;

	uint32_t new_request_internal (RequestOut& rq, IdPolicy id_policy);

	uint32_t new_request_oneway_internal (IdPolicy id_policy)
	{
		RequestId id;
		for (;;) {
			id = get_new_id (id_policy);
			auto ins = map_.insert (id);
			if (ins.second) {
				// Ensure that id is unique but remove it from the map.
				map_.remove (ins.first);
				map_.release_node (ins.first);
				break;
			} else
				map_.release_node (ins.first);
		}
		return id;
	}

	Nirvana::Core::CoreRef <RequestOut> remove_request_internal (uint32_t request_id) NIRVANA_NOEXCEPT;

	RequestMap map_;
	std::atomic <IdGenType> last_id_;

	static Nirvana::Core::StaticallyAllocated <OutgoingRequests> singleton_;
};

}
}

#endif
