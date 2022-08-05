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

StaticallyAllocated <ImplStatic <CodeSetConverter> > default_;

void CodeSetConverter::marshal_string (IDL::String& s, bool move, StreamOut& out)
{
	Base::marshal_string (s, move, out);
}

void CodeSetConverter::unmarshal_string (StreamIn& in, IDL::String& s)
{
	Base::unmarshal_string (in, s);
}

// Default code set converter for the wide characters.

void CodeSetConverterW::marshal_string (IDL::WString& s, bool move, StreamOut& out)
{
	Base::marshal_string (s, move, out);
}

void CodeSetConverterW::unmarshal_string (StreamIn& in, IDL::WString& s)
{
	Base::unmarshal_string (in, s);

	if (in.other_endian ()) {
		for (auto p = s.begin (), end = s.end (); p != end; ++p)
			byteswap (*p);
	}
}

void CodeSetConverterW::marshal_char (size_t count, const WChar* data, StreamOut& out)
{
	out.write (alignof (WChar), count * sizeof (WChar), data);
}

void CodeSetConverterW::unmarshal_char (StreamIn& in, size_t count, WChar* data)
{
	in.read (alignof (WChar), count * sizeof (WChar), data);
	if (in.other_endian ()) {
		for (WChar* p = data, *end = data + count; p != end; ++p)
			byteswap (*p);
	}
}

void CodeSetConverterW::marshal_char_seq (IDL::Sequence <WChar>& s, bool move, Request& out)
{
	out.marshal_char_seq (s, move);
}

void CodeSetConverterW::unmarshal_char_seq (Request& in, IDL::Sequence <WChar>& s)
{
	if (in.unmarshal_char_seq (s)) {
		for (auto p = s.begin (), end = s.end (); p != end; ++p)
			byteswap (*p);
	}
}

}
}
