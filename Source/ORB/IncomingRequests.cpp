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
#include "../Binder.h"
#include <algorithm>
#include "Messaging_policies.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Nirvana::Core::StaticallyAllocated <IncomingRequests::RequestMap> IncomingRequests::map_;

void IncomingRequests::receive (Ref <RequestIn> rq, uint64_t timestamp)
{
	if (rq->response_flags () & RequestIn::RESPONSE_EXPECTED) {
		auto ins = map_->insert (std::ref (rq->key ()), std::ref (*rq), timestamp);
		bool cancelled = false;
		if (!ins.second) {
			cancelled = !ins.first->value ().request && ins.first->value ().timestamp >= timestamp;
			if (cancelled)
				map_->remove (ins.first);
		}
		if (cancelled) {
			map_->release_node (ins.first);
			return;
		}
		if (!ins.second) { // Request id collision for this client
			map_->release_node (ins.first);
			throw BAD_PARAM ();
		}
		rq->inserted_to_map (ins.first);
	}

	// Currently we run with minimal initial deadline.
	// Now we must obtain the deadline value from the service context.
	// Execution domain will be rescheduled with new deadline
	// on entering to the POA or Binder synchronization domain.
	ExecDomain::current ().deadline (request_deadline (*rq));

	// Process special operations
	auto op = rq->operation ();

	if (static_cast <const std::string&> (op) == "FT_HB") {
		// Fault Tolerant CORBA heartbeat.
		// Object key is ignored. void FT_HB() may be called against any object.
		rq->success ();
		Nirvana::Core::Binder::heartbeat (rq->key ());
		return;
	}

	if (rq->object_key ().empty ()) {
		// DGC ping
		if (static_cast <const std::string&> (op) == "ping") {
			Nirvana::Core::Binder::complex_ping (*rq);
			return;
		} else
			throw BAD_OPERATION (MAKE_OMG_MINOR (2));
	}

	if (rq->object_key ().size () == 1) {
		// System objects
		switch (rq->object_key ().front ()) {
		case 0: // SysDomain
			if (ESIOP::is_system_domain ()) {
				rq->serve (*local2proxy (Services::bind (Services::SysDomain)));
				return;
			}
			break;

		case 1: // ProtDomain
			rq->serve (*local2proxy (Services::bind (Services::ProtDomain)));
			return;
		}
		throw OBJECT_NOT_EXIST (MAKE_OMG_MINOR (2));
	}

	// Invoke
	PortableServer::Core::POA_Root::invoke (Ref <RequestInPOA> (std::move (rq)), true);
}

void IncomingRequests::cancel (const RequestKey& key, uint64_t timestamp) noexcept
{
	auto ins = map_->insert (key, timestamp);
	if (!ins.second) {
		Nirvana::Core::Ref <RequestIn> request = std::move (ins.first->value ().request);
		if (request) {
			map_->remove (ins.first);
			request->cancel ();
		} else // Probably the old orphan cancel request with the same id
			ins.first->value ().timestamp = timestamp;
	}
	map_->release_node (ins.first);
}

inline
DeadlineTime IncomingRequests::request_deadline (const RequestIn& rq)
{
	// Define the request deadline from the call context.
	DeadlineTime deadline = INFINITE_DEADLINE;
	const IOP::ServiceContextList& sc = rq.service_context ();
	if (rq.key ().family == DomainAddress::Family::ESIOP) {
		auto context = binary_search (sc, ESIOP::CONTEXT_ID_DEADLINE);
		if (context) {
			ImplStatic <StreamInEncap> encap (std::ref (context->context_data ()));
			encap.read (alignof (DeadlineTime), sizeof (DeadlineTime), &deadline);
			if (encap.end ())
				throw BAD_PARAM ();
			if (encap.other_endian ())
				Internal::byteswap (deadline);
		}
	} else {
		auto context = binary_search (sc, IOP::INVOCATION_POLICIES);
		if (context) {
			PolicyMap policies (context->context_data ());
			{
				TimeBase::UtcT end_time;
				if (policies.get_value <Messaging::REQUEST_END_TIME_POLICY_TYPE> (end_time))
					return Chrono::deadline_from_UTC (end_time);
			}
			{
				Messaging::PriorityRange priority_range;
				if (policies.get_value <Messaging::REQUEST_PRIORITY_POLICY_TYPE> (priority_range))
					return Chrono::deadline_from_priority (priority_range.min ());
			}
		}
		context = binary_search (sc, IOP::RTCorbaPriority);
		if (context) {
			ImplStatic <StreamInEncap> encap (std::ref (context->context_data ()));
			int16_t priority;
			encap.read (alignof (int16_t), sizeof (int16_t), &priority);
			if (encap.end ())
				throw BAD_PARAM ();
			if (encap.other_endian ())
				Internal::byteswap (priority);
			deadline = Chrono::deadline_from_priority (priority);
		}
	}
	return deadline;
}

}
}
