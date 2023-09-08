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
#include "../pch.h"
#include "IncomingRequests.h"
#include "Chrono.h"
#include "POA_Root.h"
#include "../Binder.h"
#include "system_services.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Nirvana::Core::StaticallyAllocated <IncomingRequests::RequestMap> IncomingRequests::map_;

void IncomingRequests::receive (servant_reference <RequestIn> rq, uint64_t timestamp)
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
	ExecDomain::current ().deadline (rq->deadline ());

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

	if (ESIOP::is_system_domain ()) {
		Services::Service ss = system_service (rq->object_key ());
		if (ss < Services::SERVICE_COUNT) {
			rq->serve (*local2proxy (Services::bind (ss)));
			return;
		}
	}
	if (SysObjectKey <Nirvana::Core::ProtDomain>::equal (rq->object_key ())) {
		rq->serve (*local2proxy (Services::bind (Services::ProtDomain)));
		return;
	}

	// Invoke
	PortableServer::Core::POA_Root::invoke (servant_reference <RequestInPOA> (std::move (rq)), true);
}

void IncomingRequests::cancel (const RequestKey& key, uint64_t timestamp) noexcept
{
	auto ins = map_->insert (key, timestamp);
	if (!ins.second) {
		servant_reference <RequestIn> request = std::move (ins.first->value ().request);
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
