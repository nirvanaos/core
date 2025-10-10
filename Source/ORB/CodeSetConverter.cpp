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
#include "CodeSetConverter.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

// Default code set converter for the narrow characters.

void CodeSetConverter::marshal_string (IDL::String& s, bool move, StreamOut& out)
{
	out.write_string (s, move);
}

void CodeSetConverter::unmarshal_string (StreamIn& in, IDL::String& s)
{
	in.read_string (s);
}

StaticallyAllocated <ImplStatic <CodeSetConverter> > CodeSetConverter::default_;

// Default wide code set converter for GIOP 1.0.

servant_reference <CodeSetConverterW> CodeSetConverterW_1_0::get_default (bool client_side)
{
	return servant_reference <CodeSetConverterW>::create <ImplDynamic <CodeSetConverterW_1_0> > (client_side);
}

void CodeSetConverterW_1_0::marshal_string (IDL::WString& s, bool move, StreamOut& out)
{
	throw MARSHAL (marshal_minor_);
}

void CodeSetConverterW_1_0::unmarshal_string (StreamIn& in, IDL::WString& s)
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

void CodeSetConverterW_1_0::marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out)
{
	throw MARSHAL (marshal_minor_);
}

void CodeSetConverterW_1_0::unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s)
{
	throw MARSHAL ();
}

// Default wide code set converter for GIOP 1.1.

void CodeSetConverterW_1_1::marshal_string (IDL::WString& s, bool move, StreamOut& out)
{
	out.write_string (s, move);
}

void CodeSetConverterW_1_1::unmarshal_string (StreamIn& in, IDL::WString& s)
{
	in.read_string (s);
}

void CodeSetConverterW_1_1::marshal_char (size_t count, const WChar* data, StreamOut& out)
{
	out.write_c (2, count * 2, data);
}

void CodeSetConverterW_1_1::unmarshal_char (StreamIn& in, size_t count, WChar* data)
{
	in.read (2, sizeof (WChar), 2, count, data);
	if (in.other_endian ()) {
		for (WChar* p = data, *end = data + count; p != end; ++p)
			byteswap (*p);
	}
}

void CodeSetConverterW_1_1::marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out)
{
	out.write_seq (s, move);
}

void CodeSetConverterW_1_1::unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s)
{
	in.read_seq (s);
}

StaticallyAllocated <ImplStatic <CodeSetConverterW_1_1> > CodeSetConverterW_1_1::default_;

/// Default wide code set converter for GIOP 1.2.

void CodeSetConverterW_1_2::marshal_string (IDL::WString& s, bool move, StreamOut& out)
{
	out.write_size (s.size ());
	for (auto c : s) {
		write (c, out);
	}
}

void CodeSetConverterW_1_2::unmarshal_string (StreamIn& in, IDL::WString& s)
{
	size_t count = in.read_size ();
	s.clear ();
	s.reserve (count);
	for (; count; --count) {
		uint32_t c = read (in);
		if (sizeof (WChar) < sizeof (uint32_t) && ((~(uint32_t)0 << (sizeof (WChar) * 8)) & c))
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
		if (sizeof (WChar) < sizeof (uint32_t) && ((~(uint32_t)0 << (sizeof (WChar) * 8)) & c))
			throw_DATA_CONVERSION ();
		*(data++) = (WChar)c;
	}
}

void CodeSetConverterW_1_2::marshal_char_seq (IDL::Sequence <WChar>& s, bool move, StreamOut& out)
{
	out.write_size (s.size ());
	for (auto c : s) {
		write (c, out);
	}
}

void CodeSetConverterW_1_2::unmarshal_char_seq (StreamIn& in, IDL::Sequence <WChar>& s)
{
	size_t count = in.read_size ();
	s.clear ();
	s.reserve (count);
	for (; count; --count) {
		uint32_t c = read (in);
		if (sizeof (WChar) < sizeof (uint32_t) && ((~(uint32_t)0 << (sizeof (WChar) * 8)) & c))
			throw_DATA_CONVERSION ();
		s.push_back ((WChar)c);
	}
}

StaticallyAllocated <ImplStatic <CodeSetConverterW_1_2> > CodeSetConverterW_1_2::default_;

servant_reference <CodeSetConverterW> CodeSetConverterW::get_default (unsigned GIOP_minor, bool client_side)
{
	switch (GIOP_minor) {
		case 0:
			return CodeSetConverterW_1_0::get_default (client_side);
		case 1:
			return CodeSetConverterW_1_1::get_default ();
		default:
			return CodeSetConverterW_1_2::get_default ();
	}
}

}
}
