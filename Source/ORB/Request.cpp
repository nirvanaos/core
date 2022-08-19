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

Request::Request (unsigned GIOP_minor, bool client_side) :
	code_set_conv_ (CodeSetConverter::get_default ()),
	code_set_conv_w_ (CodeSetConverterW::get_default (GIOP_minor, client_side))
{}

void Request::set_out_size ()
{
	size_t size = stream_out_->size () - sizeof (GIOP::MessageHeader_1_3);
	if (sizeof (size_t) > sizeof (uint32_t) && size > numeric_limits <uint32_t>::max ())
		throw IMP_LIMIT ();
	((GIOP::MessageHeader_1_3*)stream_out_->header (sizeof (GIOP::MessageHeader_1_3)))->message_size ((uint32_t)size);
}

void Request::unmarshal_end ()
{
	if (stream_in_) {
		size_t more_data = !stream_in_->end ();
		stream_in_ = nullptr;
		if (more_data > 7) // 8-byte alignment is ignored
			throw MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
	}
}

}
}
