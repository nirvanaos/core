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

using namespace std;
using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

RequestOut::RequestOut (const IOP::ObjectKey& object_key, const IDL::String& operation,
	unsigned GIOP_minor, unsigned response_flags, IOP::ServiceContextList&& context,
	CoreRef <StreamOut>&& stream) :
	RequestGIOP (GIOP_minor, true),
	exec_domain_ (nullptr),
	id_ (response_flags ? OutgoingRequests::new_request (*this)
		: OutgoingRequests::new_request_oneway ())
{
	// While request in map, exec_domain_ is not nullptr.
	// For the oneway requests, exec_domain_ is nullptr.
	if (response_flags)
		exec_domain_ = &ExecDomain::current ();

	assert (stream);
	stream_out_ = move (stream);
	stream_out_->write_message_header (GIOP_minor, GIOP::MsgType::Request);

	switch (GIOP_minor) {
		case 0: {
			GIOP::RequestHeader_1_0 hdr;
			hdr.service_context (move (context));
			hdr.request_id (id_);
			hdr.response_expected (response_flags != 0);
			hdr.object_key (object_key);
			hdr.operation (operation);
			Type <GIOP::RequestHeader_1_0>::marshal_out (hdr, _get_ptr ());
		} break;

		case 1: {
			GIOP::RequestHeader_1_1 hdr;
			hdr.service_context (move (context));
			hdr.request_id (id_);
			hdr.response_expected (response_flags != 0);
			hdr.object_key (object_key);
			hdr.operation (operation);
			Type <GIOP::RequestHeader_1_1>::marshal_out (hdr, _get_ptr ());
		} break;

		default: {
			GIOP::RequestHeader_1_2 hdr;
			hdr.request_id (id_);
			hdr.response_flags (response_flags);
			hdr.target ().object_key (object_key);
			hdr.operation (operation);
			hdr.service_context (move (context));
			Type <GIOP::RequestHeader_1_2>::marshal_out (hdr, _get_ptr ());
		} break;
	}
}

RequestOut::~RequestOut ()
{
	// While request in map, exec_domain_ is not nullptr.
	// For the oneway requests, exec_domain_ is nullptr.
	if (exec_domain_)
		OutgoingRequests::remove_request (id_);
}

}
}
