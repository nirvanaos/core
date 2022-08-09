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
using namespace Nirvana::Core;
using namespace std;

namespace CORBA {

using namespace Internal;

namespace Core {

Request::Request (CoreRef <CodeSetConverterW>&& cscw) :
	code_set_conv_ (CodeSetConverter::get_default ()),
	code_set_conv_w_ (move (cscw))
{}

bool Request::marshal_seq_begin (size_t element_count)
{
	if (!marshal_op ())
		return false;

	if (sizeof (size_t) > sizeof (uint32_t) && element_count > std::numeric_limits <uint32_t>::max ())
		throw IMP_LIMIT ();
	uint32_t count = (uint32_t)element_count;
	stream_out_->write (alignof (uint32_t), sizeof (count), &count);

	return true;
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
	if (!marshal_seq_begin (element_count))
		return;

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

void Request::marshal_string_encoded (IDL::String& s, bool move)
{
	marshal_string_t (s, move);
}

void Request::unmarshal_string_encoded (IDL::String& s)
{
	unmarshal_string_t (s);
}

void Request::marshal_string_encoded (IDL::WString& s, bool move)
{
	marshal_string_t (s, move);
}

void Request::unmarshal_string_encoded (IDL::WString& s)
{
	unmarshal_string_t (s);
	if (stream_in_->other_endian ())
		for (IDL::WString::iterator p = s.begin (), end = s.end (); p != end; ++p) {
			Type <WChar>::byteswap (*p);
		}
}

}
}
