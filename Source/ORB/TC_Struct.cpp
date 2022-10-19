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

TC_Struct::TC_Struct (TCKind kind, IDL::String&& id, IDL::String&& name, Members&& members) :
	Impl (kind, std::move (id), std::move (name)),
	members_ (std::move (members))
{
	size_t off = 0, align = 1;
	bool cdr = true;
	TC_Ref self (_get_ptr (), this);
	for (auto& m : members_) {
		m.type.replace_recursive_placeholder (id_, self);
		size_t a = m.type->n_align ();
		if (align < a)
			align = a;
		off = Nirvana::round_up (off, a);
		m.offset = off;
		off += m.type->n_size ();
		if (cdr && !m.type->n_is_CDR ())
			cdr = false;
	}
	size_ = Nirvana::round_up (off, align);
	align_ = align;
	is_CDR_ = cdr;
}

bool TC_Struct::equivalent_no_alias (TypeCode::_ptr_type other) const
{
	if (members_.size () != other->member_count ())
		return false;
	for (ULong i = 0, cnt = (ULong)members_.size (); i < cnt; ++i) {
		if (!members_ [i].type->equivalent (other->member_type (i)))
			return false;
	}
	return true;
}

void TC_Struct::byteswap (void* p, size_t count) const
{
	for (Octet* pv = (Octet*)p; count; pv += size_, --count) {
		for (const auto& m : members_) {
			m.type->n_byteswap (pv + m.offset, 1);
		}
	}
}

bool TC_Struct::mark () NIRVANA_NOEXCEPT
{
	if (!TC_ComplexBase::mark ())
		return false;
	for (auto& m : members_) {
		m.type.mark ();
	}
	return true;
}

bool TC_Struct::set_recursive (const IDL::String& id, const TC_Ref& ref) NIRVANA_NOEXCEPT
{
	for (auto& m : members_) {
		m.type.replace_recursive_placeholder (id, ref);
	}
	return false;
}

}
}
