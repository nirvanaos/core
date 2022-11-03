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
#include "RequestOutESIOP.h"

using namespace CORBA;

namespace ESIOP {

void RequestOut::invoke ()
{
	if (!stream_out_)
		throw BAD_INV_ORDER ();
	Request msg (current_domain_id (), static_cast <StreamOutSM&> (*stream_out_), id_);
	domain_->send_message (&msg, sizeof (msg));
	stream_out_ = nullptr;
}

void RequestOut::cancel ()
{
	if (cancel_internal ()) {
		CancelRequest msg (current_domain_id (), id_);
		domain_->send_message (&msg, sizeof (msg));
	}
}

}
