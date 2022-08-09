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
#include "StreamReply.h"

using namespace std;

namespace Nirvana {
namespace ESIOP {

void StreamReply::write (size_t align, size_t size, void* data, size_t& allocated_size)
{
	if (small_ptr_) {
		uint8_t* p = round_up (small_ptr_, align);
		if (allocated_size || p > end (small_buffer_) || (size_t)(end (small_buffer_) - p) < size) {
			// Switch to StreamOutSM
			StreamOutSM::initialize ();
			ptrdiff_t cb = small_ptr_ - begin (small_buffer_);
			small_ptr_ = nullptr;
			if (cb > 0) {
				size_t zero = 0;
				StreamOutSM::write (1, cb, small_buffer_, zero);
			}
			StreamOutSM::write (align, size, data, allocated_size);
		} else
			small_ptr_ = real_copy ((const uint8_t*)data, (const uint8_t*)data + size, p);
	} else
		StreamOutSM::write (align, size, data, allocated_size);
}

size_t StreamReply::size () const
{
	if (small_ptr_)
		return small_ptr_ - begin (small_buffer_);
	else
		return StreamOutSM::size ();
}

void* StreamReply::header (size_t hdr_size)
{
	if (small_ptr_) {
		assert (hdr_size < sizeof (small_buffer_));
		return small_buffer_;
	} else
		return StreamOutSM::header (hdr_size);
}

void StreamReply::rewind (size_t hdr_size)
{
	if (small_ptr_) {
		assert (hdr_size < sizeof (small_buffer_));
		small_ptr_ = small_buffer_ + hdr_size;
	} else
		StreamOutSM::rewind (hdr_size);
}

void StreamReply::send (uint32_t request_id) NIRVANA_NOEXCEPT
{
	try {
		if (small_ptr_) {
			assert (REPLY_HEADERS_SIZE <= small_ptr_ - small_buffer_);
			const uint8_t* p = small_buffer_ + REPLY_HEADERS_SIZE;
			size_t size = small_ptr_ - p;
			ReplyImmediate reply (request_id, p, size);
			other_domain ().send_message (&reply, sizeof (reply));
		} else {
			Reply reply (StreamOutSM::get_shared ());
			other_domain ().send_message (&reply, sizeof (reply));
			StreamOutSM::detach ();
		}
	} catch (...) {
	}
}

}
}
