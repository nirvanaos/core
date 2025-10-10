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

namespace CORBA {
namespace Core {

void RequestOutESIOP::invoke ()
{
	pre_invoke (IdPolicy::ANY);
	StreamOutSM& stm = static_cast <StreamOutSM&> (*stream_out_);
	ESIOP::Request msg (ESIOP::current_domain_id (), stm, id ());
	domain ()->send_message (&msg, sizeof (msg));
	// After successfull sending the message we detach the output data.
	// Now it is responsibility of the target domain to release it.
	stm.detach ();
	stream_out_ = nullptr;
	security_context_.detach ();
}

void RequestOutESIOP::cancel ()
{
	if (cancel_internal ()) {
		ESIOP::CancelRequest msg (ESIOP::current_domain_id (), id ());
		domain ()->send_message (&msg, sizeof (msg));
	}
}

}
}
