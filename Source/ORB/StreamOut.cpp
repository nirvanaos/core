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
#include "StreamOut.h"

using namespace Nirvana;

namespace CORBA {
namespace Core {

void StreamOut::write_message_header (unsigned GIOP_minor, GIOP::MsgType msg_type)
{
	GIOP::MessageHeader_1_3 hdr;
	hdr.magic ({ 'G', 'I', 'O', 'P' });
	hdr.GIOP_version (GIOP::Version (1, (Octet)GIOP_minor));
	hdr.flags (endian::native == endian::little ? 1 : 0);
	hdr.message_type ((Octet)msg_type);
	write_c (1, sizeof (hdr), &hdr);
}

void StreamOut::write_size (size_t size)
{
	if (sizeof (size_t) > sizeof (uint32_t) && size > std::numeric_limits <uint32_t>::max ())
		throw IMP_LIMIT ();
	uint32_t count = (uint32_t)size;
	write_c (alignof (uint32_t), sizeof (count), &count);
}

void StreamOut::write_seq (size_t align, size_t element_size, size_t element_count, void* data,
	size_t& allocated_size)
{
	write_size (element_count);
	if (element_count)
		write (align, element_size * element_count, data, allocated_size);
}

void StreamOut::write_tagged (const IOP::TaggedProfileSeq& seq)
{
	write_size (seq.size ());
	for (const IOP::TaggedProfile& profile : seq) {
		IOP::ProfileId tag = profile.tag ();
		write_c (alignof (IOP::ProfileId), sizeof (tag), &tag);
		write_seq (profile.profile_data ());
	}
}

void StreamOut::write_id_name (TypeCode::_ptr_type tc)
{
	IDL::String s = tc->id ();
	write_string (s, true);
	s = tc->name ();
	write_string (s, true);
}

}
}
