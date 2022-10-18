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
#include "TC_Enum.h"

namespace CORBA {
namespace Core {

TC_Enum::TC_Enum (IDL::String&& id, IDL::String&& name, Members&& members) NIRVANA_NOEXCEPT :
	Impl (TCKind::tk_enum, std::move (id), std::move (name)),
	members_ (std::move (members))
{}

void TC_Enum::n_copy (void* dst, const void* src) const
{
	Internal::check_pointer (dst);
	Internal::check_pointer (src);
	Internal::ABI_enum val = *(const Internal::ABI_enum*)src;
	if (val >= members_.size ())
		throw BAD_PARAM (MAKE_OMG_MINOR (25));
	*(Internal::ABI_enum*)dst = val;
}

void TC_Enum::n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const
{
	Internal::check_pointer (src);
	check (src, count);
	rq->marshal (alignof (Internal::ABI_enum), sizeof (Internal::ABI_enum) * count, src);
}

void TC_Enum::check (const void* p, size_t count) const
{
	Internal::ABI_enum last = (Internal::ABI_enum)members_.size ();
	for (const Internal::ABI_enum* pv = (const Internal::ABI_enum*)p, *end = pv + count; pv != end; ++pv) {
		if (*pv >= last)
			throw BAD_PARAM (MAKE_OMG_MINOR (25));
	}
}

}
}
