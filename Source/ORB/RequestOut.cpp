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
#include "RequestOut.h"
#include "CodeSetConverter.h"
#include "../AtomicCounter.h"

using namespace std;
using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {

using namespace Internal;

namespace Core {

static AtomicCounter <false> request_id (0);

RequestOut::RequestOut (unsigned GIOP_minor, CoreRef <StreamOut>&& stream) :
	Request (CodeSetConverterW::get_default (GIOP_minor, false)),
	id_ ((uint32_t)request_id.increment ())
{
	assert (stream);
	stream_out_ = move (stream);
	stream_out_->write_message_header (GIOP::MsgType::Request, GIOP_minor);
}

}
}
