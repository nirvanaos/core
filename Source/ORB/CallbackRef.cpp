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
#include "CallbackRef.h"
#include "Pollable.h"
#include "call_handler.h"
#include "ProxyManager.h"
#include "../unrecoverable_error.h"
#include <signal.h>

namespace CORBA {
using namespace Internal;
namespace Core {

CallbackRef::CallbackRef (Internal::Interface::_ptr_type callback, const ProxyManager& proxy, Internal::OperationIndex op) :
	handler_op_idx_ (0)
{
	if (callback) {
		const CORBA::Internal::StringView <Char> iid (callback->_epv ().interface_id);

		RepId::CheckResult ch = RepId::check (RepIdOf <CORBA::Pollable>::id, iid);
		if (ch != RepId::CheckResult::OTHER_INTERFACE) {
			if (ch != RepId::CheckResult::COMPATIBLE)
				throw BAD_PARAM ();
			callback_ = static_cast <Pollable&> (
				static_cast <Bridge <CORBA::Pollable>&> (*&callback)).callback ();
			return;
		}

		Messaging::ReplyHandler::_ptr_type handler = Messaging::ReplyHandler::_check (&callback);
		Object::_ptr_type obj = handler;
		handler_op_idx_ = proxy.find_handler_operation (op, obj);
		callback_ = obj;
	}
}

void CallbackRef::call (const Operation& metadata, IORequest::_ptr_type rq) noexcept
{
	Interface::_ref_type cb = std::move (callback_);
	assert (cb);
	if (!cb)
		return;

	if (is_handler ())
		call_handler (metadata, rq, static_cast <Object*> (&Interface::_ptr_type (cb)), handler_op_idx_);
	else {
		try {
			static_cast <RequestCallback*> (&Interface::_ptr_type (cb))->completed (rq);
		} catch (...) {
			// We have no cases to handle this situation.
			unrecoverable_error (SIGABRT);
			// TODO: Log
		}
	}
}

}
}
