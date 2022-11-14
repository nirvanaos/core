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
#include "RequestOut.h"
#include "OutgoingRequests.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestOut::RequestOut (unsigned GIOP_minor, unsigned response_flags) :
	RequestGIOP (GIOP_minor, true, response_flags),
	id_ (0),
	status_ (Status::IN_PROGRESS)
{
	// While request in map, exec_domain_ is not nullptr.
	// For the oneway and async requests, exec_domain_ is nullptr.
	if ((response_flags & 3) && !(response_flags & IOReference::REQUEST_ASYNC))
		exec_domain_ = &ExecDomain::current ();
}

RequestOut::~RequestOut ()
{}

void RequestOut::write_header (IOP::ObjectKey& object_key, IDL::String& operation, IOP::ServiceContextList& context)
{
	stream_out_->write_message_header (GIOP_minor_, GIOP::MsgType::Request);

	switch (GIOP_minor_) {
		case 0: {
			GIOP::RequestHeader_1_0 hdr;
			hdr.service_context ().swap (context);
			hdr.request_id (id_);
			hdr.response_expected (response_flags_ != 0);
			hdr.object_key ().swap (object_key);
			hdr.operation ().swap (operation);
			Type <GIOP::RequestHeader_1_0>::marshal_out (hdr, _get_ptr ());
			hdr.service_context ().swap (context);
			hdr.object_key ().swap (object_key);
			hdr.operation ().swap (operation);
		} break;

		case 1: {
			GIOP::RequestHeader_1_1 hdr;
			hdr.service_context ().swap (context);
			hdr.request_id (id_);
			hdr.response_expected (response_flags_ != 0);
			hdr.object_key ().swap (object_key);
			hdr.operation ().swap (operation);
			Type <GIOP::RequestHeader_1_1>::marshal_out (hdr, _get_ptr ());
			hdr.service_context ().swap (context);
			hdr.object_key ().swap (object_key);
			hdr.operation ().swap (operation);
		} break;

		default: {
			GIOP::RequestHeader_1_2 hdr;
			hdr.request_id (id_);
			hdr.response_flags ((Octet)response_flags_);
			hdr.target ().object_key ().swap (object_key);
			hdr.operation ().swap (operation);
			hdr.service_context ().swap (context);
			Type <GIOP::RequestHeader_1_2>::marshal_out (hdr, _get_ptr ());
			hdr.service_context ().swap (context);
			hdr.target ().object_key ().swap (object_key);
			hdr.operation ().swap (operation);
		}
	}
}

void RequestOut::set_reply (unsigned status, IOP::ServiceContextList&& context,
	CoreRef <StreamIn>&& stream)
{
	assert (response_flags_ & 3); // No oneway
	status_ = (Status)status;
	if (Status::NO_EXCEPTION == status_ && !(response_flags_ & 2)) {
		assert (false); // No data was expected, but received. Ignore it.
		finalize ();
		return;
	}
	stream_in_ = std::move (stream);
	if (Status::NO_EXCEPTION == status_ && (FLAG_PREUNMARSHAL & response_flags_)) {
		// Preunmarshal data.
		CoreRef <RequestLocalBase> pre = CoreRef <RequestLocalBase>::
			create <RequestLocalImpl <RequestLocalBase> > (memory (), 3);

	} else if (Status::USER_EXCEPTION == status_) {
		// Preunmarshal user exception.
		CoreRef <RequestLocalBase> pre = CoreRef <RequestLocalBase>::
			create <RequestLocalImpl <RequestLocalBase> > (memory (), 3);
		Any exc;
		Type <Any>::unmarshal (_get_ptr (), exc);
		Type <Any>::marshal_out (exc, pre->_get_ptr ());
		preunmarshaled_ = std::move (pre);
	}
}

bool RequestOut::unmarshal (size_t align, size_t size, void* data)
{
	if (preunmarshaled_)
		return preunmarshaled_->unmarshal (align, size, data);
	else
		return RequestGIOP::unmarshal (align, size, data);
}

bool RequestOut::marshal_op ()
{
	return true;
}

void RequestOut::set_exception (Any& e)
{
	throw BAD_INV_ORDER ();
}

void RequestOut::set_system_exception (const Char* rep_id, uint32_t minor, CompletionStatus completed) NIRVANA_NOEXCEPT
{
	status_ = Status::SYSTEM_EXCEPTION;
	try {
		ImplStatic <StreamOutEncap> sout;
		sout.write_string_c (rep_id);
		SystemException::_Data data{ minor, completed };
		sout.write_c (alignof (SystemException::_Data), sizeof (SystemException::_Data), &data);
		stream_in_ = CoreRef <StreamIn>::create <ImplDynamic <StreamInEncapData> > (std::move (sout.data ()));
	} catch (...) {}
}

void RequestOut::success ()
{
	throw BAD_INV_ORDER ();
}

bool RequestOut::cancel_internal ()
{
	if (!(response_flags_ & 3))
		throw BAD_INV_ORDER ();

	if (CORBA::Core::OutgoingRequests::remove_request (id_)) {
		status_ = Status::CANCELLED;
		return true;
	}
	return false;
}

bool RequestOut::is_exception () const NIRVANA_NOEXCEPT
{
	return Status::SYSTEM_EXCEPTION == status_ || Status::USER_EXCEPTION == status_;
}

bool RequestOut::completed () const NIRVANA_NOEXCEPT
{
	return status_ >= Status::NO_EXCEPTION;
}

bool RequestOut::wait (uint64_t timeout)
{
	throw NO_IMPLEMENT ();
}

}
}
