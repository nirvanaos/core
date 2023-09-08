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
#include "../pch.h"
#include "RequestInESIOP.h"
#include "StreamOutSMReply.h"
#include "../Binder.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

servant_reference <StreamOut> RequestInESIOP::create_output ()
{
	auto domain = Binder::get_domain (key ().address.esiop);
	servant_reference <StreamOut> ret = servant_reference <StreamOut>::create <ImplDynamic <StreamOutSMReply> > (
		std::ref (*domain));
	domain_ = std::move (domain);
	return ret;
}

void RequestInESIOP::set_exception (Any& e)
{
	if (e.type ()->kind () != TCKind::tk_except)
		throw BAD_PARAM (MAKE_OMG_MINOR (21));

	try {
		if (finalize ()) {
			std::aligned_storage <sizeof (SystemException), alignof (SystemException)>::type buf;
			SystemException& ex = (SystemException&)buf;
			if (e >>= ex) {
				if (stream_out ())
					static_cast <StreamOutSMReply&> (*stream_out ()).system_exception (request_id (), ex);
				else {
					ESIOP::ReplySystemException reply (request_id (), ex);
					ESIOP::send_error_message (key ().address.esiop, &reply, sizeof (reply));
				}
			} else {
				Base::set_exception (e);
				static_cast <StreamOutSMReply&> (*stream_out ()).send (request_id ());
			}
		}
	} catch (...) {
		stream_out_ = nullptr;
		throw;
	}
	stream_out_ = nullptr;
}

void RequestInESIOP::success ()
{
	if (!finalize ())
		return;

	Base::success ();
	static_cast <StreamOutSMReply&> (*stream_out ()).send (request_id ());
	stream_out_ = nullptr;
	post_send_success ();
}

}
}
