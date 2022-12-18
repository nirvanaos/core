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
#include "RequestInESIOP.h"
#include "StreamInSM.h"
#include "OutgoingRequests.h"
#include "IncomingRequests.h"
#include "../Runnable.h"
#include "../CoreObject.h"
#include "../Chrono.h"

using namespace CORBA;
using namespace CORBA::Core;

using namespace Nirvana;
using namespace Nirvana::Core;

namespace ESIOP {

/// Receive request Runnable
class NIRVANA_NOVTABLE ReceiveRequest :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	ReceiveRequest (const Request& msg) NIRVANA_NOEXCEPT :
		timestamp_ (Nirvana::Core::Chrono::deadline_clock ()),
		data_ ((void*)msg.GIOP_message),
		client_id_ (msg.client_domain),
		request_id_ (msg.request_id)
	{}

	virtual void run () override;
	virtual void on_crash (const siginfo& signal) NIRVANA_NOEXCEPT override;

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
		IncomingRequests::receive (CoreRef <CORBA::Core::RequestIn>::create <RequestIn> (
			client_id_, minor, std::move (in)), timestamp_);
	} catch (const SystemException& ex) {
		if (request_id_) {
			// Responce expected
			ReplySystemException reply (request_id_, ex);
			send_error_message (client_id_, &reply, sizeof (reply));
		}
	}
}

void ReceiveRequest::on_crash (const siginfo& signal) NIRVANA_NOEXCEPT
{
	if (request_id_) {
		// Responce expected
		Exception::Code ec;
		if (signal.si_excode != Exception::EC_NO_EXCEPTION)
			ec = (Exception::Code)signal.si_excode;
		else
			ec = SystemException::EC_UNKNOWN;
		ReplySystemException reply (request_id_, ec);
		send_error_message (client_id_, &reply, sizeof (reply));
	}
}

/// Cancel request Runnable
class NIRVANA_NOVTABLE ReceiveCancel :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	ReceiveCancel (const CancelRequest& msg) NIRVANA_NOEXCEPT :
		timestamp_ (Nirvana::Core::Chrono::deadline_clock ()),
		client_id_ (msg.client_domain),
		request_id_ (msg.request_id)
	{}

	virtual void run () override;

private:
	uint64_t timestamp_;
	ProtDomainId client_id_;
	uint32_t request_id_;
};

void ReceiveCancel::run ()
{
	IncomingRequests::cancel (RequestKey (client_id_, request_id_), timestamp_);
}

/// Receive reply Runnable
class NIRVANA_NOVTABLE ReceiveReply :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	ReceiveReply (const Reply& msg) NIRVANA_NOEXCEPT :
		data_ ((void*)msg.GIOP_message),
		request_id_ (msg.request_id)
	{}

	virtual void run () override;

private:
	void* data_;
	uint32_t request_id_;
};

void ReceiveReply::run ()
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
		assert (GIOP::MsgType::Reply == (GIOP::MsgType)msg_hdr.message_type ());
		assert ((msg_hdr.flags () & 2) == 0); // Framentation is not allowed in ESIOP.

		// In the ESIOP we do not use the message size to allow > 4GB data transferring.
		// in->set_size (msg_hdr.message_size ());

		// Create and receive the request
		unsigned minor = msg_hdr.GIOP_version ().minor ();
		OutgoingRequests::receive_reply (minor, std::move (in));
	} catch (const SystemException& ex) {
		OutgoingRequests::set_system_exception (request_id_, ex._rep_id (), ex.minor (), ex.completed ());
	}
}

/// Receive reply immediate Runnable
class NIRVANA_NOVTABLE ReceiveReplyImmediate :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	ReceiveReplyImmediate (const ReplyImmediate& msg)
		NIRVANA_NOEXCEPT :
		request_id_ (msg.request_id),
		size_and_flag_ (msg.flags)
	{
		std::copy (msg.data, msg.data + ReplyImmediate::MAX_DATA_SIZE, data_);
	}

	virtual void run () override;

private:
	uint32_t request_id_;
	unsigned size_and_flag_;
	uint8_t data_ [ReplyImmediate::MAX_DATA_SIZE];
};

class StreamInImmediate :
	public StreamInEncap,
	public UserObject
{
protected:
	StreamInImmediate (unsigned size_and_flag, const uint8_t* data) :
		StreamInEncap (data_, data_ + (size_and_flag & 0x7F))
	{
		std::copy ((const uint8_t*)data, (const uint8_t*)data + ReplyImmediate::MAX_DATA_SIZE, data_);
		if (PLATFORMS_ENDIAN_DIFFERENT)
			little_endian (size_and_flag & MessageHeader::FLAG_LITTLE_ENDIAN);
	}

private:
	uint8_t data_ [ReplyImmediate::MAX_DATA_SIZE];
};

void ReceiveReplyImmediate::run ()
{
	try {
		OutgoingRequests::receive_reply (1, CoreRef <StreamIn>::create <ImplDynamic <StreamInImmediate> >
			(size_and_flag_, data_));
	} catch (const SystemException& ex) {
		OutgoingRequests::set_system_exception (request_id_, ex._rep_id (), ex.minor (), ex.completed ());
	}
}

/// Receive system exception Runnable
class NIRVANA_NOVTABLE ReceiveSystemException :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	ReceiveSystemException (const ReplySystemException& msg)
		NIRVANA_NOEXCEPT :
		completed_ ((CompletionStatus)msg.completed),
		code_ (msg.code),
		minor_ (msg.minor),
		request_id_ (msg.request_id)
	{}

	virtual void run () override;

private:
	CompletionStatus completed_;
	Exception::Code code_;
	uint32_t minor_;
	uint32_t request_id_;
};

void ReceiveSystemException::run ()
{
	OutgoingRequests::set_system_exception (request_id_,
		SystemException::_get_exception_entry (code_)->rep_id, minor_, completed_);
}

/// Shutdown Runnable
class NIRVANA_NOVTABLE ReceiveShutdown :
	public Runnable,
	public CoreObject // Must be created quickly
{
protected:
	virtual void run () override;
};

void ReceiveShutdown::run ()
{
	Scheduler::shutdown ();
}

void dispatch_message (MessageHeader& message) NIRVANA_NOEXCEPT
{
	switch (message.message_type) {
		case MessageType::REQUEST: {
			const auto& msg = Request::receive (message);
			try {
				ExecDomain::async_call (Chrono::make_deadline (INITIAL_REQUEST_DEADLINE_LOCAL),
					CoreRef <Runnable>::create <ImplDynamic <ReceiveRequest> > (std::ref (msg)),
					g_core_free_sync_context, nullptr);
			} catch (const SystemException& ex) {
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

		case MessageType::REPLY: {
			const auto& msg = Reply::receive (message);
			try {
				ExecDomain::async_call (Chrono::make_deadline (INITIAL_REQUEST_DEADLINE_LOCAL),
					CoreRef <Runnable>::create <ImplDynamic <ReceiveReply> > (std::ref (msg)),
					g_core_free_sync_context, nullptr);
			} catch (const SystemException& ex) {
				// Not enough memory?
				// Create and destruct object in stack to release stream memory.
				{
					ImplStatic <StreamInSM> tmp ((void*)msg.GIOP_message);
				}
				OutgoingRequests::set_system_exception (msg.request_id, ex._rep_id (),
					ex.minor (), ex.completed ());
			}
		} break;

		case MessageType::REPLY_IMMEDIATE: {
			const auto& msg = ReplyImmediate::receive (message);
			try {
				ExecDomain::async_call (Chrono::make_deadline (INITIAL_REQUEST_DEADLINE_LOCAL),
					CoreRef <Runnable>::create <ImplDynamic <ReceiveReplyImmediate> > (std::ref (msg)),
					g_core_free_sync_context, nullptr);
			} catch (const SystemException& ex) {
				OutgoingRequests::set_system_exception (msg.request_id, ex._rep_id (), ex.minor (), ex.completed ());
			}
		} break;

		case MessageType::REPLY_SYSTEM_EXCEPTION: {
			const auto& msg = ReplySystemException::receive (message);
			try {
				ExecDomain::async_call (Chrono::make_deadline (INITIAL_REQUEST_DEADLINE_LOCAL),
					CoreRef <Runnable>::create <ImplDynamic <ReceiveSystemException> > (std::ref (msg)),
					g_core_free_sync_context, nullptr);
			} catch (...) {
				OutgoingRequests::set_system_exception (msg.request_id,
					SystemException::_get_exception_entry (msg.code)->rep_id, msg.minor,
					(CompletionStatus)msg.completed);
			}
		} break;

		case MessageType::CANCEL_REQUEST: {
			const auto& msg = CancelRequest::receive (message);
			try {
				ExecDomain::async_call (Chrono::make_deadline (CANCEL_REQUEST_DEADLINE),
					CoreRef <Runnable>::create <ImplDynamic <ReceiveCancel> > (std::ref (msg)),
					g_core_free_sync_context, &g_shared_mem_context);
			} catch (...) {
			}
		} break;

		case MessageType::SHUTDOWN:
			try {
				ExecDomain::async_call (INFINITE_DEADLINE,
					CoreRef <Runnable>::create <ImplDynamic <ReceiveShutdown> > (),
					g_core_free_sync_context, &g_shared_mem_context);
			} catch (...) {
			}
			break;
	}
}

}
