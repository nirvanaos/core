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

TC_Union::TC_Union (IDL::String&& id, IDL::String&& name, TypeCode::_ref_type&& discriminator_type, Long default_index) :
	Impl (TCKind::tk_union, std::move (id), std::move (name)),
	discriminator_type_ (std::move (discriminator_type)),
	default_index_ (default_index)
{
	discriminator_size_ = discriminator_type_->n_size ();
}

void TC_Union::set_members (Members&& members)
{
	members_ = std::move (members);
	size_t align = discriminator_type_->n_align ();
	size_t size = discriminator_size_;
	TC_Ref self (_get_ptr (), this);
	for (auto& m : members_) {
		m.type.replace_recursive_placeholder (id_, self);
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

bool TC_Union::mark () NIRVANA_NOEXCEPT
{
	if (!TC_ComplexBase::mark ())
		return false;
	for (auto& m : members_) {
		m.type.mark ();
	}
	return true;
}

bool TC_Union::set_recursive (const IDL::String& id, const TC_Ref& ref) NIRVANA_NOEXCEPT
{
	for (auto& m : members_) {
		m.type.replace_recursive_placeholder (id, ref);
	}
	return false;
}

bool TC_Union::equivalent_members (TypeCode::_ptr_type other)
{
	if (members_.size () != other->member_count ())
		return false;
	ORB::TypeCodePair tcp (&Servant::_get_ptr (), &other, nullptr);
	if (!ORB::type_code_pair_push (tcp))
		return true;

	bool ret = true;
	try {
		for (ULong i = 0, cnt = (ULong)members_.size (); i < cnt; ++i) {
			const Member& m = members_ [i];
			if (!m.type->equivalent (other->member_type (i))) {
				ret = false;
				break;
			}
			ULongLong left = 0, right = 0;
			m.label.type ()->n_copy (&left, m.label.data ());
			Any a_label = other->member_label (i);
			a_label.type ()->n_copy (&right, a_label.data ());
			if (left != right)
				return false;
		}
	} catch (...) {
		ORB::type_code_pair_pop ();
		throw;
	}
	ORB::type_code_pair_pop ();
	return ret;
}

}
}
