/// \file
/// Platform domain environment specific inter-ORB protocol.
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
#ifndef NIRVANA_ORB_CORE_ESIOP_H_
#define NIRVANA_ORB_CORE_ESIOP_H_
#pragma once

#include <Port/ESIOP.h>

namespace Nirvana {

/// Environment-specific inter-ORB protocol.
/// Implements communication between protection domains
/// in one system domain.
namespace ESIOP {

/// Message type
enum MessageType
{
	REQUEST, ///< GIOP Request or LocateRequest message
	REPLY, ///< GIOP Reply message
	REPLY_IMMEDIATE, ///< GIOP Reply with small data
	REPLY_SYSTEM_EXCEPTION, ///< GIOP Reply with system exception
	CANCEL_REQUEST, ///< GIOP CancelRequest
	LOCATE_REPLY, ///< GIOP LocateReply

	/// Number of the ESIOP messages.
	/// Host system may add own message types.
	MESSAGES_CNT
};

/// A message header
struct MessageHeader
{
	uint8_t message_type;  ///< Message type

	MessageHeader ()
	{}

	MessageHeader (uint8_t type) :
		message_type (type)
	{}
};

/// GIOP Request
struct Request : MessageHeader
{
	/// Sender protection domain id.
	ProtDomainId client_domain;

	/// The GIOP message in the recipient memory.
	SharedMemPtr GIOP_message;
};

/// GIOP Reply
struct Reply : MessageHeader
{
	/// The GIOP message in the recipient memory.
	SharedMemPtr GIOP_message;
};

/// If GIOP Reply contains system exception, it can be sent without allocation of shared memory.
struct ReplySystemException : MessageHeader
{
	/// Completion status
	uint8_t completed;

	/// The exception code
	int16_t code;

	/// The exception minor code
	uint32_t minor;

	/// The request id
	uint32_t request_id;
};

/// GIOP CancelRequest
struct CancelRequest : MessageHeader
{
	/// Sender protection domain id.
	ProtDomainId client_domain;

	/// The request id
	uint32_t request_id;
};

/// GIOP LocateReply
struct LocateReply : MessageHeader
{
	/// The request id
	uint32_t request_id;

	/// Locate status
	uint32_t locate_status;
};

/// The message buffer enough for any message
union MessageBuffer
{
	Request request;
	Reply reply;
	ReplySystemException system_exception;
	CancelRequest cancel_request;
	LocateReply locate_reply;
};

/// If reply data size is zero or small, the reply can be sent without
/// allocation of the shared memory.
struct ReplyImmediate : MessageHeader
{
	uint8_t data_size;
	static const size_t MAX_DATA_SIZE = sizeof (MessageBuffer) - sizeof (MessageHeader) - 1 - sizeof (uint32_t);
	uint8_t data [MAX_DATA_SIZE];
	uint32_t request_id;
};

static_assert (sizeof (MessageBuffer) == sizeof (ReplyImmediate), "sizeof (MessageBuffer) == sizeof (ReplyImmediate)");

/// Message dispatch function.
/// Must be called from the portability layer.
void dispatch_message (const MessageHeader& message);

/// Initialize ESIOP on startup.
void initialize ();

/// Terminate ESIOP on shutdown.
void terminate () NIRVANA_NOEXCEPT;

}
}

#endif
