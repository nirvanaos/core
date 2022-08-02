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
#ifndef NIRVANA_ORB_CORE_MESSAGE_H_
#define NIRVANA_ORB_CORE_MESSAGE_H_
#pragma once

#include <CORBA/SystemException.h>
#include <Port/ESIOP.h>

namespace Nirvana {

/// Environment-specific inter-ORB protocol.
/// Implements communication between protection domains
/// in one system domain.
namespace ESIOP {

/// Nirvana ESIOP message.
/// Can be sent between different protection domains inside one
/// system domain.
struct Message
{
	/// Message type
	enum Type : uint16_t
	{
		REQUEST, ///< GIOP Request or LocateRequest message
		REPLY, ///< GIOP Reply message
		SYSTEM_EXCEPTION, ///< GIOP Reply with system exception
		CANCEL_REQUEST, ///< GIOP CancelRequest
		LOCATE_REPLY, ///< GIOP LocateReply

		/// Number of the ESIOP messages.
		/// Host system may add own message types.
		MESSAGES_CNT
	};

	/// A message header
	struct Header
	{
		Type  type;  ///< Message type

		Header ()
		{}

		Header (Type t) :
			type (t)
		{}
	};

	/// DataPtr points to recipient protection domain memory.
	/// Sender allocates the message data via OtherMemory interface.
	/// Core::Port::MaxPlatformPtr size is enough to store memory address
	/// for any supported platform.
	typedef MaxPlatformPtr DataPtr;

	/// GIOP Request
	struct Request : Header
	{
		/// Sender protection domain address.
		ProtDomainHandle client_address;

		/// User security token
		UserToken user;

		/// The request data (GIOP message header) in the recipient memory.
		DataPtr data_ptr;
	};

	/// GIOP Reply
	struct Reply : Header
	{
		/// Flags
		uint16_t flags;

		/// If data sent immediate, Reply::IMMEDIATE_DATA is set in flags.
		static const uint8_t IMMEDIATE_DATA = 0x01;

		/// The request id
		uint32_t request_id;

		/// If reply data size is quite small, it can be sent without allocation of shared memory.
		static const size_t IMMEDIATE_DATA_SIZE = sizeof (Request) - sizeof (request_id);

		union
		{
			/// Valid if flags & IMMEDIATE_DATA == 0
			DataPtr data_ptr;

			/// Valid if flags & IMMEDIATE_DATA != 0
			uint8_t immediate_data [IMMEDIATE_DATA_SIZE];
		};
	};

	/// If GIOP Reply contains system exception, it can be sent without allocation of shared memory.
	struct SystemException : Header
	{
		/// The request id
		uint32_t request_id;

		/// The exception code
		int32_t code;

		// The exception data
		CORBA::SystemException::_Data data;
	};

	/// GIOP CancelRequest
	struct CancelRequest : Header
	{
		/// The client address
		ProtDomainId client_address;

		/// The request id
		uint32_t request_id;
	};

	/// GIOP LocateReply
	struct LocateReply : Header
	{
		/// The request id
		uint32_t request_id;
		uint32_t locate_status;
	};

	/// The message buffer
	union Buffer
	{
		Request request;
		Reply reply;
		SystemException system_exception;
		CancelRequest cancel_request;
	};
};

}
}

#endif
