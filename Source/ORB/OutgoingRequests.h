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

	static uint32_t new_request (RequestOut& rq, RequestOut::IdPolicy id_policy)
	{
		return singleton_->new_request_internal (rq, id_policy);
	}

	static uint32_t new_request_oneway (RequestOut::IdPolicy id_policy)
	{
		return singleton_->new_request_oneway_internal (id_policy);
	}

	static servant_reference <RequestOut> remove_request (uint32_t request_id) NIRVANA_NOEXCEPT
	{
		return singleton_->remove_request_internal (request_id);
	}

	static void reply (uint32_t request_id, Nirvana::Core::CoreRef <StreamIn>&& data);

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

	RequestId get_new_id (RequestOut::IdPolicy id_type) NIRVANA_NOEXCEPT
	{
		IdGenType id;
		do {
			switch (id_type) {
				case RequestOut::IdPolicy::ANY:
					id = last_id_.fetch_add (1, std::memory_order_release) + 1;
					break;
				case RequestOut::IdPolicy::EVEN: {
					IdGenType last = last_id_.load (std::memory_order_acquire);
					IdGenType id;
					do {
						id = last + 2 - (last & 1);
					} while (!last_id_.compare_exchange_weak (last, id));
				} break;
				case RequestOut::IdPolicy::ODD: {
					IdGenType last = last_id_.load (std::memory_order_acquire);
					IdGenType id;
					do {
						id = last + 1 + (last & 1);
					} while (!last_id_.compare_exchange_weak (last, id));
				} break;
			}
		} while (0 == id);
		return (RequestId)id;
	}

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
		servant_reference <RequestOut> request;
	};

	// The request map
	typedef Nirvana::Core::SkipList <RequestVal, SKIP_LIST_LEVELS> RequestMap;

	uint32_t new_request_internal (RequestOut& rq, RequestOut::IdPolicy id_type)
	{
		RequestId id;
		for (;;) {
			id = get_new_id (id_type);
			auto ins = map_.insert (id, std::ref (rq));
			map_.release_node (ins.first);
			if (ins.second)
				break;
		}
		return id;
	}

	uint32_t new_request_oneway_internal (RequestOut::IdPolicy id_type)
	{
		RequestId id;
		for (;;) {
			id = get_new_id (id_type);
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

	servant_reference <RequestOut> remove_request_internal (uint32_t request_id) NIRVANA_NOEXCEPT;

	RequestMap map_;
	std::atomic <IdGenType> last_id_;

	static Nirvana::Core::StaticallyAllocated <OutgoingRequests> singleton_;
};

}
}

#endif
