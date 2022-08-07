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
#include <CORBA/CORBA.h>
#include "ESIOP.h"
#include "RequestIn.h"
#include "StreamInSM.h"
#include "IncomingRequests.h"
#include "../Runnable.h"
#include "../CoreObject.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace std;

namespace Nirvana {

using namespace Core;

namespace ESIOP {

/// ESIOP incoming request.
class NIRVANA_NOVTABLE RequestIn : 
	public RequestIn_1_1
{
	typedef RequestIn_1_1 Base;
public:
	RequestIn (ProtDomainId client_id, CoreRef <StreamIn>&& in) :
		Base (move (in)),
		client_id_ (client_id)
	{}

private:
	ProtDomainId client_id_;
};

/// Receive request
class NIRVANA_NOVTABLE ReceiveRequest :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	ReceiveRequest (ProtDomainId client_id, uint32_t request_id, void* data) :
		data_ (data),
		client_id_ (client_id),
		request_id_ (request_id)
	{}

	virtual void run () override;

private:
	void* data_;
	ProtDomainId client_id_;
	uint32_t request_id_;
};

void ReceiveRequest::run ()
{
	servant_reference <RequestIn> rq;
	try {
		// Create input stream
		CoreRef <StreamIn> in;
		try {
			in = CoreRef <StreamIn>::create <ImplDynamic <StreamInSM> > (data_);
		} catch (...) {
			// Not enough memory?
			// Create and destruct object in stack to release stream memory.
			ImplStatic <StreamInSM> tmp (data_);
			throw;
		}

		// Read GIOP message header
		GIOP::MessageHeader_1_1 msg_hdr;
		in->read (1, sizeof (msg_hdr), &msg_hdr);
		assert (equal (begin (msg_hdr.magic ()), end (msg_hdr.magic ()), "GIOP"));

		// We always use GIOP 1.1 in ESIOP for the native marshaling of wide characters.
		assert ((msg_hdr.GIOP_version ().major () == 1) && (msg_hdr.GIOP_version ().minor () == 1));
		assert (GIOP::MsgType::Request == (GIOP::MsgType)msg_hdr.message_type ());
		assert ((msg_hdr.flags () & 2) == 0); // Framentation is not allowed in ESIOP.

		// Set endian
		in->little_endian (msg_hdr.flags () & 1);

		// Set size
		uint32_t msg_size = msg_hdr.message_size ();
		if (in->other_endian ())
			msg_size = byteswap (msg_size);
		in->set_size (msg_size);

		// Create request
		rq = make_reference <RequestIn> (client_id_, move (in));
	} catch (const CORBA::SystemException& ex) {
		ReplySystemException reply (request_id_, ex);
		send_error_message (client_id_, &reply, sizeof (reply));
		return;
	}

	// We have a request, receive it
	IncomingRequests::receive (client_id_, *rq);
}

/// Cancel request
class NIRVANA_NOVTABLE Cancel :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	Cancel (ProtDomainId client_id, uint32_t request_id) :
		client_id_ (client_id),
		request_id_ (request_id)
	{}

	virtual void run () override;

private:
	ProtDomainId client_id_;
	uint32_t request_id_;
};

void Cancel::run ()
{
	IncomingRequests::cancel (client_id_, request_id_);
}

void dispatch_message (const MessageHeader& message) NIRVANA_NOEXCEPT
{
	switch (message.message_type) {
		case MessageType::REQUEST: {
			const auto& msg = static_cast <const Request&> (message);
			try {
				ExecDomain::async_call (INITIAL_REQUEST_DEADLINE_LOCAL,
					CoreRef <Runnable>::create <ImplDynamic <ReceiveRequest> > (msg.client_domain, msg.request_id, (void*)msg.GIOP_message),
					g_core_free_sync_context);
			} catch (const CORBA::SystemException& ex) {
				// Not enough memory?
				// Create and destruct object in stack to release stream memory.
				{
					ImplStatic <StreamInSM> tmp ((void*)msg.GIOP_message);
				}
				// Highly likely we are out of resources here, so we shouldn't use the asyncronous call.
				ReplySystemException reply (msg.request_id, ex);
				send_error_message (msg.client_domain, &reply, sizeof (reply));
			}
		} break;

		case MessageType::CANCEL_REQUEST: {
			const auto& msg = static_cast <const CancelRequest&> (message);
			try {
				ExecDomain::async_call (CANCEL_REQUEST_DEADLINE,
					CoreRef <Runnable>::create <ImplDynamic <Cancel> > (msg.client_domain, msg.request_id),
					g_core_free_sync_context);
			} catch (...) {
			}
		} break;

	}
}

}
}
