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
#include "../pch.h"
#include "StreamOutSMReply.h"

using namespace Nirvana;

namespace CORBA {
namespace Core {

void StreamOutSMReply::write (size_t align, size_t element_size, size_t CDR_element_size,
	size_t element_count, void* data, size_t& allocated_size)
{
	if (small_ptr_) {
		size_t size = element_size * element_count;
		if (!size)
			return;

		if (!data)
			throw_BAD_PARAM ();

		uint8_t* p = round_up (small_ptr_, align);
		if (allocated_size || p > std::end (small_buffer_) || (size_t)(std::end (small_buffer_) - p) < size) {
			switch_to_base ();
		} else {
			small_ptr_ = real_copy ((const uint8_t*)data, (const uint8_t*)data + size, p);
			return;
		}
	}
	StreamOutSM::write (align, element_size, CDR_element_size, element_count, data, allocated_size);
}

void StreamOutSMReply::switch_to_base ()
{
	StreamOutSM::initialize ();
	ptrdiff_t cb = small_ptr_ - std::begin (small_buffer_);
	small_ptr_ = nullptr;
	if (cb > 0) {
		size_t zero = 0;
		StreamOutSM::write (1, cb, cb, 1, small_buffer_, zero);
	}
}

size_t StreamOutSMReply::size () const
{
	if (small_ptr_)
		return small_ptr_ - std::begin (small_buffer_);
	else
		return StreamOutSM::size ();
}

void* StreamOutSMReply::header (size_t hdr_size)
{
	if (small_ptr_) {
		assert (hdr_size < sizeof (small_buffer_));
		return small_buffer_;
	} else
		return StreamOutSM::header (hdr_size);
}

void StreamOutSMReply::rewind (size_t hdr_size)
{
	if (small_ptr_) {
		assert (hdr_size < sizeof (small_buffer_));
		small_ptr_ = small_buffer_ + hdr_size;
	} else
		StreamOutSM::rewind (hdr_size);
}

void StreamOutSMReply::send (uint32_t request_id) noexcept
{
	try {
		if (small_ptr_) {
			// Check that the reply header context is empty
			const GIOP::MessageHeader_1_3* msg_hdr = (const GIOP::MessageHeader_1_3*)small_buffer_;
			uint32_t context_cnt;
			if (msg_hdr->GIOP_version ().minor () <= 1)
				context_cnt = *(const uint32_t*)(msg_hdr + 1);
			else
				context_cnt = *((const uint32_t*)(msg_hdr + 1) + 2);

			if (context_cnt)
				switch_to_base ();
			else {
				// Send reply with immediate data
				const uint8_t* p = small_buffer_ + REPLY_HEADERS_SIZE;
				assert (small_ptr_ >= p);
				size_t size = small_ptr_ - p;
				ESIOP::ReplyImmediate reply (request_id, p, size);
				other_domain ().send_message (&reply, sizeof (reply));
				return;
			}
		}

		ESIOP::Reply reply (*this, request_id);
		other_domain ().send_message (&reply, sizeof (reply));
		// After successfull sending the message we detach the output data.
		// Now it is responsibility of the target domain to release it.
		StreamOutSM::detach ();
	} catch (...) {
	}
}

}
}
