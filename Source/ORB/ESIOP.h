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

#include <CORBA/CORBA.h>
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

	/// The request id.
	/// If response is not expected, request_id = 0.
	/// The real request id is never be 0.
	uint32_t request_id;
};

/// GIOP Reply
struct Reply : MessageHeader
{
	/// The GIOP message in the recipient memory.
	SharedMemPtr GIOP_message;

	Reply (SharedMemPtr mem) :
		MessageHeader (REPLY),
		GIOP_message (mem)
	{}
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

	ReplySystemException (uint32_t rq_id, const CORBA::SystemException& ex) :
		MessageHeader (REPLY_SYSTEM_EXCEPTION),
		completed ((uint8_t)ex.completed ()),
		code ((int16_t)ex.__code ()),
		minor (ex.minor ()),
		request_id (rq_id)
	{}

	ReplySystemException (uint32_t rq_id, CORBA::SystemException::Code code) :
		MessageHeader (REPLY_SYSTEM_EXCEPTION),
		completed ((uint8_t)CORBA::CompletionStatus::COMPLETED_MAYBE),
		code ((int16_t)code),
		minor (0),
		request_id (rq_id)
	{}
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
	uint8_t data_size_and_flag;
	static const size_t MAX_DATA_SIZE = sizeof (MessageBuffer) - sizeof (MessageHeader) - 1 - sizeof (uint32_t);
	static_assert (MAX_DATA_SIZE < 127, "MAX_DATA_SIZE < 127");

	uint8_t data [MAX_DATA_SIZE];
	uint32_t request_id;

	ReplyImmediate (uint32_t rq_id, const void* data, size_t size) :
		MessageHeader (REPLY_IMMEDIATE),
		data_size_and_flag ((uint8_t)(size << 1)),
		request_id (rq_id)
	{
		assert (size <= MAX_DATA_SIZE);
		if (endian::native == endian::little)
			data_size_and_flag |= 1;
	}
};

static_assert (sizeof (MessageBuffer) == sizeof (ReplyImmediate), "sizeof (MessageBuffer) == sizeof (ReplyImmediate)");

/// Message dispatch function.
/// Called by the postman from portability layer.
void dispatch_message (const MessageHeader& message) NIRVANA_NOEXCEPT;

/// Vendor service context ID. Used only in ESIOP messages.
const uint32_t VSCID = 0xFFFFFF;

///@{ Service context IDs

/// Local deadline.
/// context_data contains the local DeadlineTime, 8 bytes.
const uint32_t CONTEXT_ID_DEADLINE = (VSCID << 8) | 0;

///@}

}
}

#endif
