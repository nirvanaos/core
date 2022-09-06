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
#include "Reference.h"
#include "RequestLocalPOA.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

Internal::IORequest::_ref_type Reference::create_request (OperationIndex op, UShort flags)
{
	if (is_object_op (op))
		return ProxyManager::create_request (op, flags);

	check_create_request (op, flags);

	UShort response_flags = flags & 3;
	if (flags & REQUEST_ASYNC)
		return make_pseudo <RequestLocalImpl <RequestLocalAsyncPOA> > (std::ref (*this), op,
			object_key (), response_flags);
	else
		return make_pseudo <RequestLocalImpl <RequestLocalPOA> > (std::ref (*this), op,
			object_key (), response_flags);
}

void Reference::marshal (StreamOut& out)
{
	ReferenceLocal::marshal (primary_interface_id (), out);
}

}
}
