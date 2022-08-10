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

namespace CORBA {
namespace Core {

void StreamIn::read_message_header (GIOP::MessageHeader_1_3& msg_hdr)
{
	read (1, sizeof (msg_hdr), &msg_hdr);
	if (!std::equal (msg_hdr.magic ().begin (), msg_hdr.magic ().end (), "GIOP"))
		throw MARSHAL ();
	little_endian (msg_hdr.flags () & 1);
	if (other_endian_)
		msg_hdr.message_size (Nirvana::byteswap (msg_hdr.message_size ()));
}

}
}
