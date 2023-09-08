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
#include "TC_Struct.h"

namespace CORBA {
namespace Core {

void TC_Struct::set_members (Members&& members)
{
	members_ = std::move (members);
	size_t off = 0;
	bool cdr = true;
	align_ = 1;
	CDR_size_ = 0;
	size_t mem_align = 1;
	auto it = members_.begin ();
	if (it != members_.end ()) {
		align_ = mem_align = it->type->n_align ();
		for (;;) {
			it->offset = off;
			off += it->type->n_aligned_size ();
			if (cdr && !it->type->n_is_CDR ())
				cdr = false;
			if (members_.end () == ++it)
				break;
			size_t a = it->type->n_align ();
			if (mem_align < a)
				mem_align = a;
			off = Nirvana::round_up (off, a);
		}
		CDR_size_ = members_.back ().offset + members_.back ().type->n_CDR_size ();
	}
	aligned_size_ = Nirvana::round_up (off, mem_align);
	is_CDR_ = cdr;
}

void TC_Struct::byteswap (void* p, size_t count) const
{
	for (Octet* pv = (Octet*)p; count; pv += aligned_size_, --count) {
		for (const auto& m : members_) {
			m.type->n_byteswap (pv + m.offset, 1);
		}
	}
}

bool TC_Struct::mark () noexcept
{
	if (!TC_ComplexBase::mark ())
		return false;
	for (auto& m : members_) {
		m.type.mark ();
	}
	return true;
}

bool TC_Struct::set_recursive (const IDL::String& id, const TC_Ref& ref) noexcept
{
	for (auto& m : members_) {
		m.type.replace_recursive_placeholder (id, ref);
	}
	return false;
}

}
}
