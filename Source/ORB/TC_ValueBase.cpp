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
#include "TC_ValueBase.h"

namespace CORBA {
namespace Core {

TC_ValueBase::TC_ValueBase (TCKind kind, IDL::String&& id, IDL::String&& name) NIRVANA_NOEXCEPT :
	TC_Complex <TC_RefBase> (kind, std::move (id), std::move (name))
{}

void TC_ValueBase::marshal (const void* src, size_t count, Internal::IORequest_ptr rq, bool out)
{
	Internal::check_pointer (src);
	for (Internal::Interface* const* p = reinterpret_cast <Internal::Interface* const*> (src);
		count; ++p, --count) {
		rq->marshal_value (*p, out);
	}
}

}
}