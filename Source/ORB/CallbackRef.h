/// \file
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
#ifndef NIRVANA_ORB_CORE_CALLBACKREF_H_
#define NIRVANA_ORB_CORE_CALLBACKREF_H_
#pragma once

#include "ProxyManager.h"

namespace CORBA {
namespace Core {

class CallbackRef
{
public:
	CallbackRef (ProxyManager& proxy, Internal::OperationIndex op, Internal::Interface::_ptr_type callback);

	bool is_handler () const noexcept
	{
		return handler_op_idx_ != 0;
	}

	void call (Internal::IORequest::_ptr_type rq, Internal::OperationIndex op);

private:
	Internal::Interface::_ref_type callback_;
	Internal::OperationIndex handler_op_idx_;
};

}
}

#endif
