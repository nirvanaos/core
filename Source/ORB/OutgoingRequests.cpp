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
#include "OutgoingRequests.h"
#include "RequestOut.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

StaticallyAllocated <OutgoingRequests::RequestMap> OutgoingRequests::map_;

void OutgoingRequests::new_request (RequestOut& rq, RequestOut::IdPolicy id_policy)
{
	RequestId id = rq.id ();
	for (;;) {
		auto ins = map_->insert (id, std::ref (rq));
		map_->release_node (ins.first);
		if (ins.second)
			break;
		id = RequestOut::get_new_id (id_policy);
	}
	if (rq.id () != id)
		rq.id (id);
}

void OutgoingRequests::new_request_oneway (RequestOut& rq, RequestOut::IdPolicy id_policy)
{
	RequestId id = rq.id ();
	for (;;) {
		auto ins = map_->insert (id);
		if (ins.second) {
			// Ensure that id is unique but remove it from the map.
			map_->remove (ins.first);
			map_->release_node (ins.first);
			break;
		} else
			map_->release_node (ins.first);
		id = RequestOut::get_new_id (id_policy);
	}
	if (rq.id () != id)
		rq.id (id);
}

servant_reference <RequestOut> OutgoingRequests::remove_request (RequestOut::RequestId request_id) noexcept
{
	RequestMap::NodeVal* p = map_->find_and_delete (request_id);
	servant_reference <RequestOut> ret;
	if (p) {
		ret = std::move (p->value ().request);
		map_->release_node (p);
	}
	return ret;
}

void OutgoingRequests::set_system_exception (uint32_t request_id, SystemException::Code code,
	uint32_t minor, CompletionStatus completed) noexcept
{
	servant_reference <RequestOut> rq = remove_request (request_id);
	if (rq)
		rq->set_system_exception (code, minor, completed);
}

void OutgoingRequests::on_reply_exception (RequestOut& rq, const SystemException& ex,
	GIOP::ReplyStatusType status) noexcept
{
	CompletionStatus completed = ex.completed ();
	if (GIOP::ReplyStatusType::NO_EXCEPTION == status)
		completed = CompletionStatus::COMPLETED_YES;
	rq.set_system_exception (ex.__code (), ex.minor (), completed);
}

void OutgoingRequests::receive_reply (unsigned GIOP_minor, servant_reference <StreamIn>&& stream)
{
	IOP::ServiceContextList context1;
	if (GIOP_minor <= 1)
		stream->read_tagged (context1);
	uint32_t request_id = stream->read32 ();
	servant_reference <RequestOut> rq = remove_request (request_id);
	if (rq) {
		uint32_t status = (uint32_t)(-1);
		try {
			status = stream->read32 ();
			receive_reply_internal (*rq, GIOP_minor, (RequestId)request_id, status, context1, std::move (stream));
		} catch (const SystemException& ex) {
			on_reply_exception (*rq, ex, (GIOP::ReplyStatusType)status);
		}
	}
}

void OutgoingRequests::receive_reply_internal (RequestOut& rq, unsigned GIOP_minor, RequestId request_id,
	uint32_t status, const IOP::ServiceContextList& context1, servant_reference <StreamIn>&& stream)
{
	ExecDomain& ed = ExecDomain::current ();
	servant_reference <MemContext> mc = &rq.memory ();
	ed.mem_context_swap (mc);
	try {
		IOP::ServiceContextList context;
		if (GIOP_minor > 1)
			stream->read_tagged (context);
		else
			context = context1;
		rq.set_reply (status, std::move (context), std::move (stream));
	} catch (...) {
		ed.mem_context_swap (mc);
		throw;
	}
	ed.mem_context_swap (mc);
}

}
}
