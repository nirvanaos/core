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
#include "call_handler.h"
#include "ExceptionHolder.h"
#include "ProxyManager.h"
#include "remarshal_output.h"

namespace CORBA {
using namespace Internal;
namespace Core {

void call_handler (const Operation& metadata, IORequest::_ptr_type rq, Object::_ptr_type handler,
	Internal::OperationIndex op_idx) noexcept
{
	try {
		ProxyManager* handler_proxy = ProxyManager::cast (handler);
		assert (handler_proxy);
		IORequest::_ref_type rq_handler;
		Any exc;
		if (rq->get_exception (exc)) {
			rq_handler = handler_proxy->create_request (op_idx + 1, 0, nullptr);
			try {
				Nirvana::Core::ImplStatic <ExceptionHolder> eh (std::ref (exc));
				rq_handler->marshal_value (Messaging::ExceptionHolder::_ptr_type (&eh));
			} catch (const SystemException& ex) {
				exc <<= ex;
				try {
					Nirvana::Core::ImplStatic <ExceptionHolder> eh (std::ref (exc));
					rq_handler->marshal_value (Messaging::ExceptionHolder::_ptr_type (&eh));
				} catch (...) {
					rq_handler->marshal_value (nullptr);
				}
			}
		} else {
			rq_handler = handler_proxy->create_request (op_idx, 0, nullptr);
			remarshal_output (metadata, rq, rq_handler);
		}
		rq_handler->invoke ();
	} catch (...) {
		// TODO: Log
	}
}

}
}
