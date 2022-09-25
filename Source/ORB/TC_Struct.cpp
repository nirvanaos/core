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
#include "TC_Struct.h"

namespace CORBA {
namespace Core {

TC_Struct::TC_Struct (IDL::String id, IDL::String name, Members&& members) NIRVANA_NOEXCEPT :
	Impl (TCKind::tk_struct, std::move (id), std::move (name)),
	members_ (std::move (members))
{
	size_t off = 0, align = 1;
	bool cdr = true;
	for (auto& m : members_) {
		size_t a = m.type->n_align ();
		if (align < a)
			align = a;
		off = Nirvana::round_up (off, a);
		m.offset = off;
		off += m.type->n_size ();
		if (cdr && !m.type->n_is_CDR ())
			cdr = false;
	}
	size_ = off;
	align_ = align;
	is_CDR_ = cdr;
}

}
}
