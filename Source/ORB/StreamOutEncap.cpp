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
#include "StreamOutEncap.h"

using namespace Nirvana;

namespace CORBA {
namespace Core {

StreamOutEncap::StreamOutEncap (bool no_endian) :
	chunk_begin_ (0)
#ifdef _DEBUG
	, endian_ (false)
#endif
{
	if (!no_endian) {
		buffer_.resize (1);
		buffer_.front () = endian::native == endian::little ? 1 : 0;
#ifdef _DEBUG
		endian_ = true;
#endif
	}
}

void StreamOutEncap::write (size_t align, size_t size, void* data, size_t& allocated_size)
{
	size_t begin = round_up (buffer_.size (), align);
	size_t end = begin + size;
	buffer_.resize (end);
	real_copy ((const Octet*)data, (const Octet*)data + size, buffer_.data () + begin);
}

size_t StreamOutEncap::size () const
{
	return buffer_.size ();
}

void* StreamOutEncap::header (size_t hdr_size)
{
	return buffer_.data ();
}

void StreamOutEncap::rewind (size_t hdr_size)
{
	buffer_.resize (hdr_size);
	chunk_begin_ = 0;
}

void StreamOutEncap::chunk_begin ()
{
	assert (!endian_);
	if (!chunk_begin_) {
		size_t end = round_up (buffer_.size (), (size_t)4) + 4;
		buffer_.resize (end);
		chunk_begin_ = end;
	}
}

bool StreamOutEncap::chunk_end ()
{
	assert (!endian_);
	Long cs = StreamOutEncap::chunk_size ();
	assert (cs != 0);
	if (cs > 0) {
		*(Long*)(buffer_.data () + (chunk_begin_ - 4)) = cs;
		chunk_begin_ = 0;
		return true;
	}
	return false;
}

Long StreamOutEncap::chunk_size () const
{
	assert (!endian_);
	if (chunk_begin_) {
		size_t cb = buffer_.size () - chunk_begin_;
		if (cb >= 0x7FFFFF00)
			throw MARSHAL ();
		return (Long)cb;
	} else
		return -1;
}

}
}
