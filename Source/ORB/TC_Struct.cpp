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

void TC_Struct::set_members (Members&& members)
{
	members_ = std::move (members);
	size_t off = 0;
	size_t align = 1;
	bool var_len = false;
	SizeAndAlign sa (1);
	for (auto& m : members_) {
		size_t m_align = m.type->n_align ();
		off = Nirvana::round_up (off, m_align);
		m.offset = off;
		off += m.type->n_size ();
		if (align < m_align)
			align = m_align;
		if (!var_len && is_var_len (m.type, sa))
			var_len = true;
	}
	size_ = Nirvana::round_up (off, align);
	align_ = align;

	if (var_len)
		kind_ = KIND_VARLEN;
	else if (sa.is_valid ()) {
		CDR_size_ = sa;
		kind_ = KIND_CDR;
	} else
		kind_ = KIND_FIXLEN;
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

void TC_Struct::marshal_CDR (const void* src, size_t count, Internal::IORequest_ptr rq) const
{
	for (const Octet* osrc = (const Octet*)src; count; osrc += size_, --count) {
		rq->marshal (CDR_size_.alignment, CDR_size_.size, osrc);
	}
}

}
}
