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
#include "TC_Union.h"

namespace CORBA {
namespace Core {

TC_Union::TC_Union (String&& id, String&& name, TypeCode::_ref_type&& discriminator_type,
	Long default_index, Members&& members) NIRVANA_NOEXCEPT :
	Impl (TCKind::tk_union, std::move (id), std::move (name)),
	discriminator_type_ (std::move (discriminator_type)),
	members_ (std::move (members)),
	default_index_ (default_index)
{
	discriminator_size_ = discriminator_type_->n_size ();
	size_t align = discriminator_type_->n_align ();
	size_t size = discriminator_size_;
	for (auto& m : members_) {
		size_t a = m.type->n_align ();
		if (align < a)
			align = a;
		size_t off = Nirvana::round_up (discriminator_size_, a);
		m.offset = off;
		off += m.type->n_size ();
		if (size < off)
			size = off;
	}
	size_ = size;
	align_ = align;
}

}
}
