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
#include "ESIOP.h"
#include "IncomingRequests.h"
#include "StreamInSM.h"
#include "StreamOutSM.h"

using namespace CORBA::Core;
using namespace std;

namespace Nirvana {

using namespace Core;

namespace ESIOP {

class NIRVANA_NOVTABLE StreamReply :
	public StreamOutSM
{
public:
	StreamReply (ProtDomainId client_id) :
		StreamOutSM (message_.reply_immediate.data),
		client_id_ (client_id)
	{}

private:
	ProtDomainId client_id_;

	union Message
	{
		ReplyImmediate reply_immediate;
		Reply reply;
		ReplySystemException system_exception;

		Message () :
			reply_immediate ()
		{}
	}
	message_;
};

/// ESIOP incoming request.
class NIRVANA_NOVTABLE RequestIn : 
	private ImplStatic <StreamInSM>,
	private ImplStatic <StreamReply>,
	public CORBA::Core::RequestIn_1_1
{
protected:
	using CORBA::Core::RequestIn_1_1::operator new;
	using CORBA::Core::RequestIn_1_1::operator delete;

	RequestIn (SharedMemPtr mem, ProtDomainId client_id) :
		ImplStatic <StreamInSM> ((void*)mem),
		ImplStatic <StreamReply> (client_id),
		CORBA::Core::RequestIn_1_1 (static_cast <StreamIn&> (*this), static_cast <StreamOut&> (*this))
	{}
};

void dispatch_message (const MessageHeader& message) NIRVANA_NOEXCEPT
{
	switch (message.message_type) {
		case MessageType::REQUEST: {
			const auto& msg = static_cast <const Request&> (message);

			CoreRef <CORBA::Core::RequestIn> rq = CoreRef <CORBA::Core::RequestIn>::create <ImplDynamic <RequestIn> > (msg.GIOP_message, msg.client_domain);
			GIOP::MessageHeader_1_1 msg_hdr;
			rq->stream_in ()->read (1, sizeof (msg_hdr), &msg_hdr);
			assert (equal (begin (msg_hdr.magic ()), end (msg_hdr.magic ()), "GIOP"));

			// We always use GIOP 1.1 in ESIOP for the native marshaling of wide characters.
			assert ((msg_hdr.GIOP_version ().major () == 1) && (msg_hdr.GIOP_version ().minor () == 1));
			assert (GIOP::MsgType::Request == (GIOP::MsgType)msg_hdr.message_type ());
			assert ((msg_hdr.flags () & 2) == 0); // Framentation is not allowed in ESIOP.

			uint32_t msg_size = msg_hdr.message_size ();
			if (rq->stream_in ()->other_endian ())
				msg_size = byteswap (msg_size);
			rq->stream_in ()->set_size (msg_size);

			try {
				IncomingRequests::receive (msg.client_domain, rq);
			} catch (const CORBA::SystemException& ex) {
				// Highly likely we are out of resources here, so we don't use asyncronous call.
				ReplySystemException reply (msg.request_id, ex);
				send_error_message (msg.client_domain, &reply, sizeof (reply));
			}
		} break;

		case MessageType::CANCEL_REQUEST: {
			const auto& msg = static_cast <const CancelRequest&> (message);
			IncomingRequests::cancel (msg.client_domain, msg.request_id);
		} break;

	}
}

}
}
