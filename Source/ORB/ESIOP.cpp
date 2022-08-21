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
#include "StreamInSM.h"
#include "StreamOutReply.h"
#include "IncomingRequests.h"
#include "../Runnable.h"
#include "../CoreObject.h"
#include "../Chrono.h"
#include "OtherDomains.h"

using namespace CORBA;
using namespace CORBA::Core;
using namespace std;

namespace Nirvana {

using namespace Core;

namespace ESIOP {

/// ESIOP incoming request.
class NIRVANA_NOVTABLE RequestIn :
	public CORBA::Core::RequestIn
{
	typedef CORBA::Core::RequestIn Base;

protected:
	RequestIn (ProtDomainId client_id, unsigned GIOP_minor, CoreRef <StreamIn>&& in) :
		Base (client_id, GIOP_minor, move (in))
	{}

	virtual CoreRef <StreamOut> create_output () override;
	virtual void set_exception (Any& e) override;
	virtual void success () override;
};

CoreRef <StreamOut> RequestIn::create_output ()
{
	return CoreRef <StreamOut>::create <ImplDynamic <StreamOutReply> > (key ().address.esiop);
}

void RequestIn::set_exception (Any& e)
{
	if (e.type ()->kind () != TCKind::tk_except)
		throw BAD_PARAM (MAKE_OMG_MINOR (21));

	if (!finalize ())
		return;

	aligned_storage <sizeof (SystemException), alignof (SystemException)>::type buf;
	SystemException& ex = (SystemException&)buf;
	if (e >>= ex) {
		if (stream_out ())
			static_cast <StreamOutReply&> (*stream_out ()).system_exception (request_id (), ex);
		else {
			ReplySystemException reply (request_id (), ex);
			send_error_message (key ().address.esiop, &reply, sizeof (reply));
		}
	} else {
		assert (stream_out ());
		Base::set_exception (e);
		static_cast <StreamOutReply&> (*stream_out ()).send (request_id ());
	}
}

void RequestIn::success ()
{
	if (!finalize ())
		return;

	Base::success ();
	static_cast <StreamOutReply&> (*stream_out ()).send (request_id ());
}

/// Receive request Runnable
class NIRVANA_NOVTABLE ReceiveRequest :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	ReceiveRequest (ProtDomainId client_id, uint32_t request_id, void* data) :
		timestamp_ (Core::Chrono::deadline_clock ()),
		data_ (data),
		client_id_ (client_id),
		request_id_ (request_id)
	{}

	virtual void run () override;

private:
	uint64_t timestamp_;
	void* data_;
	ProtDomainId client_id_;
	uint32_t request_id_;
};

void ReceiveRequest::run ()
{
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
		in->read_message_header (msg_hdr);

		// We do not use GIOP 1.0 in ESIOP.
		assert ((msg_hdr.GIOP_version ().major () == 1) && (msg_hdr.GIOP_version ().minor () > 0));
		assert (GIOP::MsgType::Request == (GIOP::MsgType)msg_hdr.message_type ());
		assert ((msg_hdr.flags () & 2) == 0); // Framentation is not allowed in ESIOP.

		// In the ESIOP we do not use the message size to allow > 4GB data transferring.
		// in->set_size (msg_hdr.message_size ());

		// Create and receive the request
		unsigned minor = msg_hdr.GIOP_version ().minor ();
		ImplStatic <RequestIn> request (client_id_, minor, move (in));
		IncomingRequests::receive (request, timestamp_);

	} catch (const CORBA::SystemException& ex) {
		if (request_id_) {
			// Responce expected
			ReplySystemException reply (request_id_, ex);
			send_error_message (client_id_, &reply, sizeof (reply));
		}
		return;
	}
}

/// Cancel request Runnable
class NIRVANA_NOVTABLE Cancel :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	Cancel (ProtDomainId client_id, uint32_t request_id) :
		timestamp_ (Core::Chrono::deadline_clock ()),
		client_id_ (client_id),
		request_id_ (request_id)
	{}

	virtual void run () override;

private:
	uint64_t timestamp_;
	ProtDomainId client_id_;
	uint32_t request_id_;
};

void Cancel::run ()
{
	IncomingRequests::cancel (RequestKey (client_id_, request_id_), timestamp_);
}

void dispatch_message (const MessageHeader& message) NIRVANA_NOEXCEPT
{
	switch (message.message_type) {
		case MessageType::REQUEST: {
			const auto& msg = static_cast <const Request&> (message);
			try {
				ExecDomain::async_call (Chrono::make_deadline (INITIAL_REQUEST_DEADLINE_LOCAL),
					CoreRef <Runnable>::create <ImplDynamic <ReceiveRequest> > (msg.client_domain, msg.request_id, (void*)msg.GIOP_message),
					g_core_free_sync_context, &g_shared_mem_context);
			} catch (const CORBA::SystemException& ex) {
				// Not enough memory?
				// Create and destruct object in stack to release stream memory.
				{
					ImplStatic <StreamInSM> tmp ((void*)msg.GIOP_message);
				}
				if (msg.request_id) { // Responce expected
					// Highly likely we are out of resources here, so we shouldn't use the asyncronous call.
					ReplySystemException reply (msg.request_id, ex);
					send_error_message (msg.client_domain, &reply, sizeof (reply));
				}
			}
		} break;

		case MessageType::CANCEL_REQUEST: {
			const auto& msg = static_cast <const CancelRequest&> (message);
			try {
				ExecDomain::async_call (Chrono::make_deadline (CANCEL_REQUEST_DEADLINE),
					CoreRef <Runnable>::create <ImplDynamic <Cancel> > (msg.client_domain, msg.request_id),
					g_core_free_sync_context, &g_shared_mem_context);
			} catch (...) {
			}
		} break;

	}
}

}
}
