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

void StreamOut::write_message_header (GIOP::MsgType msg_type, unsigned GIOP_minor)
{
	GIOP::MessageHeader_1_3 hdr;
	hdr.magic ({ 'G', 'I', 'O', 'P' });
	hdr.GIOP_version ().major (1);
	hdr.GIOP_version ().minor ((Octet)GIOP_minor);
	hdr.flags (endian::native == endian::little ? 1 : 0);
	hdr.message_type ((Octet)msg_type);
	write (1, sizeof (hdr), &hdr);
}

}
}
