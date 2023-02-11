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
#include "OutgoingRequests.h"
#include "RequestOut.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

StaticallyAllocated <OutgoingRequests> OutgoingRequests::singleton_;

OutgoingRequests::RequestId OutgoingRequests::get_new_id (IdPolicy id_policy) NIRVANA_NOEXCEPT
{
	IdGenType id;
	do {
		switch (id_policy) {
			case IdPolicy::ANY:
				id = last_id_.fetch_add (1, std::memory_order_release) + 1;
				break;

			case IdPolicy::EVEN: {
				IdGenType last = last_id_.load (std::memory_order_acquire);
				IdGenType id;
				do {
					id = last + 2 - (last & 1);
				} while (!last_id_.compare_exchange_weak (last, id));
			} break;

			case IdPolicy::ODD: {
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

inline
uint32_t OutgoingRequests::new_request_internal (RequestOut& rq, IdPolicy id_policy)
{
	RequestId id;
	for (;;) {
		id = get_new_id (id_policy);
		auto ins = map_.insert (id, std::ref (rq));
		map_.release_node (ins.first);
		if (ins.second)
			break;
	}
	return id;
}

uint32_t OutgoingRequests::new_request (RequestOut& rq, IdPolicy id_policy)
{
	return singleton_->new_request_internal (rq, id_policy);
}

inline
Ref <RequestOut> OutgoingRequests::remove_request_internal (uint32_t request_id) NIRVANA_NOEXCEPT
{
	RequestMap::NodeVal* p = map_.find_and_delete (request_id);
	Ref <RequestOut> ret;
	if (p) {
		ret = std::move (p->value ().request);
		map_.release_node (p);
	}
	return ret;
}

Ref <RequestOut> OutgoingRequests::remove_request (uint32_t request_id) NIRVANA_NOEXCEPT
{
	return singleton_->remove_request_internal (request_id);
}

void OutgoingRequests::set_system_exception (uint32_t request_id, const Char* rep_id,
	uint32_t minor, CompletionStatus completed) NIRVANA_NOEXCEPT
{
	Ref <RequestOut> rq = remove_request (request_id);
	if (rq) {
		ExecDomain& ed = ExecDomain::current ();
		ed.mem_context_replace (rq->memory ());
		rq->set_system_exception (rep_id, minor, completed);
		ed.mem_context_restore ();
	}
}

void OutgoingRequests::receive_reply (unsigned GIOP_minor, Ref <StreamIn>&& stream)
{
	IOP::ServiceContextList context1;
	uint32_t request_id;
	uint32_t status;
	if (GIOP_minor <= 1)
		stream->read_tagged (context1);
	request_id = stream->read32 ();
	status = stream->read32 ();
	receive_reply_internal (GIOP_minor, request_id, status, context1, std::move (stream));
}

void OutgoingRequests::receive_reply_internal (unsigned GIOP_minor, uint32_t request_id,
	uint32_t status, const IOP::ServiceContextList& context1, Ref <StreamIn>&& stream)
{
	Ref <RequestOut> rq = remove_request (request_id);
	if (rq) {
		ExecDomain& ed = ExecDomain::current ();
		Ref <MemContext> mc = &rq->memory ();
		ed.mem_context_swap (mc);
		try {
			IOP::ServiceContextList context;
			if (GIOP_minor > 1)
				stream->read_tagged (context);
			else
				context = context1;
			rq->set_reply (status, std::move (context), std::move (stream));
		} catch (...) {
			ed.mem_context_swap (mc);
			throw;
		}
		ed.mem_context_swap (mc);
	}
}

}
}
