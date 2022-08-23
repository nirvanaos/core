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
#include "IncomingRequests.h"
#include "Chrono.h"
#include "POA_Root.h"
#include <algorithm>

using namespace std;
using namespace Nirvana;
using namespace Nirvana::Core;

inline void PortableServer::Core::POA_Root::invoke (CORBA::Core::RequestIn& request)
{
	ExecDomain& ed = ExecDomain::current ();
	// Called from free sync context
	assert (!ed.sync_context ().sync_domain ());

	POA::_ref_type root = ServantBase::_default_POA ();
	const CORBA::Core::LocalObject* proxy = get_proxy (root);
	POA_Base* adapter = get_implementation (proxy);
	assert (adapter);
	const char* path = request.object_key ().POA_path.c_str ();
	const char* end = path + request.object_key ().POA_path.size ();

	SYNC_BEGIN (proxy->sync_context (), ed.mem_context_ptr ());
	while (path != end) {
		const char* name_end = std::find (path, end, '/');
		if (name_end == end) {
			adapter = &adapter->find_child (CORBA::Internal::StringBase <char> (path, name_end - path), true);
			path = end;
		} else {
			adapter = &adapter->find_child (IDL::String (path, name_end - path), true);
			path = name_end + 1;
		}
	}
	adapter->invoke (request.object_key ().object_id, request);

	_sync_frame.return_to_caller_context ();
	SYNC_END ();
}

namespace CORBA {
namespace Core {

Nirvana::Core::StaticallyAllocated <IncomingRequests::RequestMap> IncomingRequests::map_;

void IncomingRequests::receive (RequestIn& rq, uint64_t timestamp)
{
	try {
		ExecDomain& ed = ExecDomain::current ();
		if (rq.response_flags ()) {
			auto ins = map_->insert (ref (rq.key ()), ref (rq), timestamp);
			bool cancelled = false;
			if (!ins.second) {
				cancelled = !ins.first->value ().request && ins.first->value ().timestamp >= timestamp;
				if (cancelled)
					map_->remove (ins.first);
			}
			map_->release_node (ins.first);
			if (cancelled)
				return;
			if (!ins.second) // Request id collision for this client
				throw BAD_PARAM ();
			rq.exec_domain (&ed);
		}

		// Currently we run with minimal initial deadline.
		// Now we must obtain the deadline value from the service context.
		const IOP::ServiceContextList& sc = rq.service_context ();

		DeadlineTime deadline = INFINITE_DEADLINE;
		for (const auto& context : sc) {
			if (ESIOP::CONTEXT_ID_DEADLINE == context.context_id ()) {
				if (context.context_data ().size () != 8)
					throw BAD_PARAM ();
				deadline = *(DeadlineTime*)context.context_data ().data ();
				if (rq.stream_in ()->other_endian ())
					deadline = byteswap (deadline);
				break;
			} else if (IOP::RTCorbaPriority == context.context_id ()) {
				if (context.context_data ().size () != 2)
					throw BAD_PARAM ();
				int16_t priority = *(int16_t*)context.context_data ().data ();
				if (rq.stream_in ()->other_endian ())
					priority = byteswap ((uint16_t)priority);
				deadline = Chrono::deadline_from_priority (priority);
				break;
			}
		}

		ed.deadline (deadline);

		// Execution domain will be rescheduled with new deadline
		// on entering to the POA synchronization domain.
		PortableServer::Core::POA_Root::invoke (rq);

	} catch (Exception& ex) {
		Any any;
		try {
			any <<= move (ex);
			rq.set_exception (any);
		} catch (...) {
		}
	}
}

void IncomingRequests::cancel (const RequestKey& key, uint64_t timestamp) NIRVANA_NOEXCEPT
{
	auto ins = map_->insert (key, timestamp);
	if (!ins.second) {
		RequestIn* request = ins.first->value ().request;
		if (request) {
			map_->remove (ins.first);
			request->cancel ();
		} else // Probably the old orphan cancel request with the same id
			ins.first->value ().timestamp = timestamp;
	}
	map_->release_node (ins.first);
}

}
}
