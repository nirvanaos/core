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
//#include "RequestIn.h"
#include <algorithm>

using namespace std;
using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Nirvana::Core::StaticallyAllocated <IncomingRequests::RequestMap> IncomingRequests::map_;

void IncomingRequests::receive (const ClientAddress& source, RequestIn& rq)
{
	try {
		uint32_t request_id = rq.request_id ();
		ExecDomain& ed = ExecDomain::current ();
		auto ins = map_->insert (ref (source), request_id, &ed);
		bool cancelled = false;
		if (!ins.second) {
			cancelled = !ins.first->value ().exec_domain;
			if (cancelled)
				map_->erase (ins.first->value ());
		}
		map_->release_node (ins.first);
		if (cancelled)
			return;
		if (!ins.second) // Request id collision for this client
			throw BAD_PARAM ();

		// Currently we run with minimal initial deadline.
		// Now we must obtain the deadline value from the service context.
		const IOP::ServiceContextList& sc = rq.service_context ();

		// Stub. TODO: Implement.
		DeadlineTime deadline = INFINITE_DEADLINE;
		ed.deadline (deadline);
		ed.yield (); // Reschedule

		// Now we obtain the target object key
		rq.object_key ();

		// TODO: Implement

	} catch (Exception& ex) {
		Any any;
		try {
			any <<= move (ex);
			rq.set_exception (any);
		} catch (...) {
		}
	}
}

void IncomingRequests::cancel (const ClientAddress& source, uint32_t request_id) NIRVANA_NOEXCEPT
{
	auto ins = map_->insert (ref (source), request_id, nullptr);
	if (ins.first->value ().exec_domain) {
		// TODO: We must call exec_domain->abort () here but it is not implemented yet
		map_->erase (ins.first->value ());
		map_->release_node (ins.first);
	}
}

}
}
