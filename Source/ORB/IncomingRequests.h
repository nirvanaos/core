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
#ifndef NIRVANA_ORB_CORE_INCOMINGREQUESTS_H_
#define NIRVANA_ORB_CORE_INCOMINGREQUESTS_H_
#pragma once

#include "RequestIn.h"
#include "../SkipList.h"

namespace CORBA {
namespace Core {

class RequestIn;

/// Incoming requests manager
class IncomingRequests
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

	/// Recieve incoming request.
	/// 
	/// \param rq The input request.
	static void receive (RequestIn& rq, uint64_t timestamp);

	/// Cancel incoming request.
	/// 
	/// \param key The request key.
	static void cancel (const RequestKey& key, uint64_t timestamp) NIRVANA_NOEXCEPT;

	/// Finalize incoming request.
	/// 
	/// \param key The request key.
	/// \returns `true` if response must be sent.
	static bool finalize (const RequestKey& key) NIRVANA_NOEXCEPT
	{
		return map_->erase (std::ref (key));
	}

private:
	struct RequestVal : RequestKey
	{
		RequestVal (const RequestKey& key, RequestIn& rq, uint64_t time) NIRVANA_NOEXCEPT :
			RequestKey (key),
			timestamp (time),
			request (&rq)
		{}

		RequestVal (const RequestKey& key, uint64_t time) NIRVANA_NOEXCEPT :
			RequestKey (key),
			timestamp (time),
			request (nullptr)
		{}

		RequestVal (const RequestKey& key) NIRVANA_NOEXCEPT :
			RequestKey (key),
			request (nullptr) // Is not make sense to initialize timestamp here.
		{}

		uint64_t timestamp;
		RequestIn* request;
	};

	// The request map
	typedef Nirvana::Core::SkipList <RequestVal, SKIP_LIST_LEVELS> RequestMap;

	static Nirvana::Core::StaticallyAllocated <RequestMap> map_;
};

}
}

#endif
