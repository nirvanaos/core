/// \file
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
#ifndef NIRVANA_ORB_CORE_STREAMOUTREPLY_H_
#define NIRVANA_ORB_CORE_STREAMOUTREPLY_H_
#pragma once

#include <CORBA/CORBA.h>
#include "ESIOP.h"
#include "StreamOutSM.h"

namespace ESIOP {

class NIRVANA_NOVTABLE StreamOutReply :
	public StreamOutSM
{
public:
	// Size of GIOP MessageHeader + size of GIOP ReplyHeader with empty service_context.
	static const size_t REPLY_HEADERS_SIZE = 24;

	StreamOutReply (ProtDomainId client_id) :
		StreamOutSM (client_id),
		small_ptr_ (small_buffer_)
	{}

	void system_exception (uint32_t request_id, const CORBA::SystemException& ex) NIRVANA_NOEXCEPT
	{
		try {
			StreamOutSM::clear ();
			ReplySystemException reply (request_id, ex);
			other_domain ().send_message (&reply, sizeof (reply));
		} catch (...) {
		}
	}

	void send (uint32_t request_id) NIRVANA_NOEXCEPT;

	virtual void write (size_t align, size_t size, void* data, size_t& allocated_size) override;
	virtual size_t size () const override;
	virtual void* header (size_t hdr_size) override;
	virtual void rewind (size_t hdr_size) override;

private:
	void switch_to_base ();

private:
	uint8_t* small_ptr_;
	uint8_t small_buffer_ [REPLY_HEADERS_SIZE + ReplyImmediate::MAX_DATA_SIZE];
};

}

#endif
