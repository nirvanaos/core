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
	LOCATE_REPLY, ///< GIOP LocateReply - currently unused

	/// Number of the ESIOP messages.
	/// Host system may add own message types.
	MESSAGES_CNT
};

/// A message header
struct MessageHeader
{
	uint8_t message_type;  ///< Message type

	MessageHeader () NIRVANA_NOEXCEPT
	{}

	MessageHeader (uint8_t type) NIRVANA_NOEXCEPT :
		message_type (type)
	{}
};

/// GIOP Request
struct Request : MessageHeader
{
	/// The GIOP message in the recipient memory.
	SharedMemPtr GIOP_message;

	/// Sender protection domain id.
	ProtDomainId client_domain;

	/// The request id.
	/// If response is not expected, request_id = 0.
	/// The real request id is never be 0.
	uint32_t request_id;

	Request (ProtDomainId client, SharedMemPtr mem, uint32_t rq_id) NIRVANA_NOEXCEPT :
		MessageHeader (REQUEST),
		GIOP_message (mem),
		client_domain (client),
		request_id (rq_id)
	{}
};

/// GIOP Reply
struct Reply : MessageHeader
{
	/// The GIOP message in the recipient memory.
	SharedMemPtr GIOP_message;

	/// The request id.
	uint32_t request_id;

	Reply (SharedMemPtr mem, uint32_t rq_id) NIRVANA_NOEXCEPT :
		MessageHeader (REPLY),
		GIOP_message (mem),
		request_id (rq_id)
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

	ReplySystemException (uint32_t rq_id, const CORBA::SystemException& ex) NIRVANA_NOEXCEPT :
		MessageHeader (REPLY_SYSTEM_EXCEPTION),
		completed ((uint8_t)ex.completed ()),
		code ((int16_t)ex.__code ()),
		minor (ex.minor ()),
		request_id (rq_id)
	{}

	ReplySystemException (uint32_t rq_id, CORBA::SystemException::Code code) NIRVANA_NOEXCEPT :
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

	CancelRequest (ProtDomainId sender, uint32_t rq_id) NIRVANA_NOEXCEPT :
		MessageHeader (CANCEL_REQUEST),
		client_domain (sender),
		request_id (rq_id)
	{}
};

/// The message buffer enough for any message
union MessageBuffer
{
	Request request;
	Reply reply;
	ReplySystemException system_exception;
	CancelRequest cancel_request;
};

/// If reply data size is zero or small, the reply can be sent without
/// allocation of the shared memory.
struct ReplyImmediate : MessageHeader
{
	uint8_t data_size_and_flag;
	static const size_t MAX_DATA_SIZE = sizeof (MessageBuffer) - sizeof (MessageHeader) - 1 - sizeof (uint32_t);
	static_assert (MAX_DATA_SIZE <= 127, "MAX_DATA_SIZE <= 127");

	uint8_t data [MAX_DATA_SIZE];
	uint32_t request_id;

	ReplyImmediate (uint32_t rq_id, const void* p, size_t size) NIRVANA_NOEXCEPT :
		MessageHeader (REPLY_IMMEDIATE),
		data_size_and_flag ((uint8_t)(size << 1)),
		request_id (rq_id)
	{
		assert (size <= MAX_DATA_SIZE);
		if (Nirvana::endian::native == Nirvana::endian::little)
			data_size_and_flag |= 1;
		std::copy ((const uint8_t*)p, (const uint8_t*)p + size, data);
	}
};

static_assert (sizeof (MessageBuffer) == sizeof (ReplyImmediate), "sizeof (MessageBuffer) == sizeof (ReplyImmediate)");

/// Message dispatch function.
/// Called by the postman from portability layer.
void dispatch_message (const MessageHeader& message) NIRVANA_NOEXCEPT;

/// Other domain platform properties.
struct PlatformSizes
{
	size_t allocation_unit; ///< Shared memory ALLOCATION_UNIT.
	size_t block_size; ///< Stream block size granularity.
	size_t sizeof_pointer; ///< sizeof (void*) on target platform.
	size_t sizeof_size; ///< sizeof (size_t) on target platform.
	size_t max_size; ///< maximal size_t value.
};

/// Nirvana ORB type.
///
/// The TAG_ORB_TYPE component has an associated value of type unsigned long, encoded as a CDR
/// encapsulation, designating an ORB type ID allocated by the OMG for the ORB type of the
/// originating ORB. Anyone may register any ORB types by submitting a short (one - paragraph)
/// description of the ORB type to the OMG, and will receive a new ORB type ID in return.
/// A list of ORB type descriptions and values will be made available on the OMG web server.
const uint32_t ORB_TYPE = 0xFFFFFFFF;

/// Vendor service context ID. TODO: Obtain from OMG.
const uint32_t VSCID = 0xFFFFFF;

///@{ Service context IDs

/// Local deadline.
/// context_data contains the local DeadlineTime, 8 bytes.
/// Used only for calls inside the local system domain.
const uint32_t CONTEXT_ID_DEADLINE = (VSCID << 8) | 0;

///@}

/// Nirvana ESIOP component tag. TODO: Obtain from OMG.
/// profile_data contains the local protection domain ID.
/// If this profile is not present, the local system domain is assumed.
const uint32_t TAG_DOMAIN_ADDRESS = 0xFFFFFFFF;

/// profile_data contains single octet with IOR flags.
const uint32_t TAG_FLAGS = 0xFFFFFFFE;

/// IOR is a subgect of the Distributed Garbage Collection.
const uint8_t IOR_FLAG_DGC = 0x01;

}

#endif
