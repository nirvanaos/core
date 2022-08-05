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
#include "Request.h"

using namespace Nirvana;

namespace CORBA {
namespace Core {

Request::Request (GIOP::Version GIOP_version, bool client_side) :
	code_set_conv_ (&CodeSetConverter::get_default ()),
	code_set_conv_w_ (&CodeSetConverterW::get_default (GIOP_version.minor (), client_side))
{}

void Request::marshal_seq_begin (size_t element_count)
{
	marshal_op ();
	if (sizeof (size_t) > sizeof (uint32_t) && element_count > std::numeric_limits <uint32_t>::max ())
		throw IMP_LIMIT ();
	uint32_t count = (uint32_t)element_count;
	stream_out_->write (alignof (uint32_t), sizeof (count), &count);
}

size_t Request::unmarshal_seq_begin ()
{
	uint32_t count;
	stream_in_->read (alignof (uint32_t), sizeof (count), &count);
	if (stream_in_->other_endian ())
		byteswap (count);
	if (sizeof (size_t) < sizeof (uint32_t) && count > std::numeric_limits <size_t>::max ())
		throw IMP_LIMIT ();
	return (size_t)count;
}

void Request::marshal_seq (size_t align, size_t element_size, size_t element_count, void* data,
	size_t& allocated_size)
{
	check_align (align);
	marshal_seq_begin (element_count);

	if (element_count)
		stream_out_->write (align, element_size * element_count, data, allocated_size);
}

bool Request::unmarshal_seq (size_t align, size_t element_size, size_t& element_count, void*& data,
	size_t& allocated_size)
{
	check_align (align);

	size_t count = unmarshal_seq_begin ();

	if ((element_count = count)) {
		size_t size = element_size * element_count;
		data = stream_in_->read (align, size);
		allocated_size = size;
	} else {
		data = nullptr;
		allocated_size = 0;
	}
	return stream_in_->other_endian ();
}

}
}
