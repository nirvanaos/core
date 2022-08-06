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

// Default code set converter for the narrow characters.

void CodeSetConverter::marshal_string (IDL::String& s, bool move, Request& out)
{
	out.marshal_string_encoded (s, move);
}

void CodeSetConverter::unmarshal_string (Request& in, IDL::String& s)
{
	in.unmarshal_string_encoded (s);
}

StaticallyAllocated <ImplStatic <CodeSetConverter> > CodeSetConverter::default_;

// Default wide code set converter for GIOP 1.0.

CoreRef <CodeSetConverterW> CodeSetConverterW_1_0::get_default (bool client_side)
{
	return CoreRef <CodeSetConverterW>::create <ImplDynamic <CodeSetConverterW_1_0> > (client_side);
}

void CodeSetConverterW_1_0::marshal_string (IDL::WString& s, bool move, Request& out)
{
	throw MARSHAL (marshal_minor_);
}

void CodeSetConverterW_1_0::unmarshal_string (Request& in, IDL::WString& s)
{
	throw MARSHAL ();
}

void CodeSetConverterW_1_0::marshal_char (size_t count, const WChar* data, StreamOut& out)
{
	throw MARSHAL (marshal_minor_);
}

void CodeSetConverterW_1_0::unmarshal_char (StreamIn& in, size_t count, WChar* data)
{
	throw MARSHAL ();
}

void CodeSetConverterW_1_0::marshal_char_seq (IDL::Sequence <WChar>& s, bool move, Request& out)
{
	throw MARSHAL (marshal_minor_);
}

void CodeSetConverterW_1_0::unmarshal_char_seq (Request& in, IDL::Sequence <WChar>& s)
{
	throw MARSHAL ();
}

// Default wide code set converter for GIOP 1.1.

void CodeSetConverterW_1_1::marshal_string (IDL::WString& s, bool move, Request& out)
{
	out.marshal_string_encoded (s, move);
}

void CodeSetConverterW_1_1::unmarshal_string (Request& in, IDL::WString& s)
{
	in.unmarshal_string_encoded (s);
}

void CodeSetConverterW_1_1::marshal_char (size_t count, const WChar* data, StreamOut& out)
{
	out.write (alignof (WChar), count * sizeof (WChar), data);
}

void CodeSetConverterW_1_1::unmarshal_char (StreamIn& in, size_t count, WChar* data)
{
	in.read (alignof (WChar), count * sizeof (WChar), data);
	if (in.other_endian ()) {
		for (WChar* p = data, *end = data + count; p != end; ++p)
			byteswap (*p);
	}
}

void CodeSetConverterW_1_1::marshal_char_seq (IDL::Sequence <WChar>& s, bool move, Request& out)
{
	out.marshal_char_seq (s, move);
}

void CodeSetConverterW_1_1::unmarshal_char_seq (Request& in, IDL::Sequence <WChar>& s)
{
	if (in.unmarshal_char_seq (s)) {
		for (auto p = s.begin (), end = s.end (); p != end; ++p)
			byteswap (*p);
	}
}

StaticallyAllocated <ImplStatic <CodeSetConverterW_1_1> > CodeSetConverterW_1_1::default_;

/// Default wide code set converter for GIOP 1.2.

void CodeSetConverterW_1_2::marshal_string (IDL::WString& s, bool move, Request& out)
{
	out.marshal_seq_begin (s.size ());
	for (auto c : s) {
		write (c, *out.stream_out ());
	}
}

void CodeSetConverterW_1_2::unmarshal_string (Request& in, IDL::WString& s)
{
	s.clear ();
	size_t count = in.unmarshal_seq_begin ();
	s.reserve (count);
	for (; count; --count) {
		uint32_t c = read (*in.stream_in ());
		if (sizeof (WChar) < sizeof (uint32_t) && ((~(uint32_t)1 << (sizeof (WChar) * 8)) & c))
			throw_DATA_CONVERSION ();
		s.push_back ((WChar)c);
	}
}

void CodeSetConverterW_1_2::marshal_char (size_t count, const WChar* data, StreamOut& out)
{
	for (const WChar* end = data + count; data != end; ++data) {
		write (*data, out);
	}
}

void CodeSetConverterW_1_2::unmarshal_char (StreamIn& in, size_t count, WChar* data)
{
	for (; count; --count) {
		uint32_t c = read (in);
		if (sizeof (WChar) < sizeof (uint32_t) && ((~(uint32_t)1 << (sizeof (WChar) * 8)) & c))
			throw_DATA_CONVERSION ();
		*(data++) = (WChar)c;
	}
}

void CodeSetConverterW_1_2::marshal_char_seq (IDL::Sequence <WChar>& s, bool move, Request& out)
{
	out.marshal_seq_begin (s.size ());
	for (auto c : s) {
		write (c, *out.stream_out ());
	}
}

void CodeSetConverterW_1_2::unmarshal_char_seq (Request& in, IDL::Sequence <WChar>& s)
{
	s.clear ();
	size_t count = in.unmarshal_seq_begin ();
	s.reserve (count);
	for (; count; --count) {
		uint32_t c = read (*in.stream_in ());
		if (sizeof (WChar) < sizeof (uint32_t) && ((~(uint32_t)1 << (sizeof (WChar) * 8)) & c))
			throw_DATA_CONVERSION ();
		s.push_back ((WChar)c);
	}
}

StaticallyAllocated <ImplStatic <CodeSetConverterW_1_2> > CodeSetConverterW_1_2::default_;

}
}
