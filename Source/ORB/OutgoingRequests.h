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
#include "StreamIn.h"
#include <type_traits>

namespace CORBA {
namespace Core {

class RequestOut;

/// Outgoing requests manager
class OutgoingRequests
{
	static const unsigned SKIP_LIST_LEVELS = 10; // TODO: config.h

public:
	/// Called on system startup
	static void initialize ()
	{
		map_.construct ();
		last_id_.construct (0);
	}

	/// Called on system shutdown
	static void terminate ()
	{
		map_.destruct ();
	}

	static uint32_t new_request (RequestOut& rq);

	static void remove_request (uint32_t request_id);

	static uint32_t new_request_oneway ();

	static void reply (uint32_t request_id, Nirvana::Core::CoreRef <StreamIn>&& data);

private:
	// Request id generator.
	typedef Nirvana::Core::AtomicCounter <false> IdGen;

	// AtomicIntegral may have different sizes. We need 32-bit max, if possible.
	typedef IdGen::IntegralType AtomicIntegral;
	typedef std::conditional_t <(sizeof (AtomicIntegral) <= 4), AtomicIntegral, uint32_t> RequestId;

	static RequestId get_new_id () NIRVANA_NOEXCEPT
	{
		RequestId id = (RequestId)(last_id_->increment ());
		if (!id)
			id = (RequestId)(last_id_->increment ());
		return id;
	}

	struct RequestVal
	{
		RequestVal (RequestId id, RequestOut* rq) NIRVANA_NOEXCEPT :
			request_id (id),
			request (rq)
		{}

		RequestVal (RequestId id) NIRVANA_NOEXCEPT :
			request_id (id),
			request (nullptr)
		{}

		bool operator < (const RequestVal& rhs) const NIRVANA_NOEXCEPT
		{
			return request_id < rhs.request_id;
		}

		RequestId request_id;
		RequestOut* request;
	};

	// The request map
	typedef Nirvana::Core::SkipList <RequestVal, SKIP_LIST_LEVELS> RequestMap;

	static Nirvana::Core::StaticallyAllocated <RequestMap> map_;
	static Nirvana::Core::StaticallyAllocated <IdGen> last_id_;
};

inline
uint32_t OutgoingRequests::new_request (RequestOut& rq)
{
	RequestId id;
	for (;;) {
		id = get_new_id ();
		auto ins = map_->insert (id, &rq);
		map_->release_node (ins.first);
		if (ins.second)
			break;
	}
	return id;
}

inline
uint32_t OutgoingRequests::new_request_oneway ()
{
	RequestId id;
	for (;;) {
		id = get_new_id ();
		auto ins = map_->insert (id);
		if (ins.second) {
			// Ensure that id is unique but remove it from the map.
			map_->remove (ins.first);
			map_->release_node (ins.first);
			break;
		} else
			map_->release_node (ins.first);
	}
	return id;
}

inline
void OutgoingRequests::remove_request (uint32_t request_id)
{
	verify (map_->erase ((RequestId)request_id));
}

}
}

#endif
