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
#include "StreamIn.h"
#include <algorithm>
#include "../MemContext.h"

using namespace Nirvana;

namespace CORBA {
namespace Core {

void StreamIn::read_message_header (GIOP::MessageHeader_1_3& msg_hdr)
{
	read_one (msg_hdr);
	if (!std::equal (msg_hdr.magic ().begin (), msg_hdr.magic ().end (), "GIOP"))
		throw MARSHAL ();
	little_endian (msg_hdr.flags () & 1);
	if (other_endian_)
		msg_hdr.message_size (Nirvana::byteswap (msg_hdr.message_size ()));
}

size_t StreamIn::read_size ()
{
	uint32_t count;
	read_one (count);
	if (sizeof (size_t) < sizeof (uint32_t) && count > std::numeric_limits <size_t>::max ())
		throw IMP_LIMIT ();
	return (size_t)count;
}

bool StreamIn::read_seq (size_t align, size_t element_size, size_t CDR_element_size,
	size_t& element_count, void*& data, size_t& allocated_size)
{
	element_count = 0;
	size_t count = read_size ();

	if (count) {
		size_t size = element_size * count;
		if (allocated_size >= size)
			read (align, element_size, CDR_element_size, count, data);
		else {
			size = 0;
			void* new_data = read (align, element_size, CDR_element_size, count, size);
			if (allocated_size) {
				Nirvana::Core::MemContext::current ().heap ().release (data, allocated_size);
				allocated_size = 0;
			}
			data = new_data;
			allocated_size = size;
		}
		element_count = count;
	}
	return other_endian ();
}

void StreamIn::read_tagged (IOP::TaggedProfileSeq& seq)
{
	size_t cnt = read_size ();
	seq.resize (cnt);
	for (IOP::TaggedProfile& profile : seq) {
		read_one (profile.tag ());
		read_seq (profile.profile_data ());
	}
}

size_t StreamIn::chunk_tail () const
{
	NIRVANA_UNREACHABLE_CODE ();
	return 0;
}

Long StreamIn::skip_chunks ()
{
	NIRVANA_UNREACHABLE_CODE ();
	throw MARSHAL ();
}

}
}
