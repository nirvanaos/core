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
#include "TC_Value.h"

namespace CORBA {
namespace Core {

bool TC_Value::mark () NIRVANA_NOEXCEPT
{
	if (!TC_ValueBase::mark ())
		return false;
	for (auto& m : members_) {
		m.type.mark ();
	}
	return true;
}

bool TC_Value::set_recursive (const IDL::String& id, const TC_Ref& ref) NIRVANA_NOEXCEPT
{
	for (auto& m : members_) {
		m.type.replace_recursive_placeholder (id, ref);
	}
	return false;
}

bool TC_Value::equivalent_members (TypeCode::_ptr_type other)
{
	if (members_.size () != other->member_count ())
		return false;
	ORB::TypeCodePair tcp (&Servant::_get_ptr (), &other, nullptr);
	if (!g_ORB->type_code_pair_push (tcp))
		return true;

	bool ret = true;
	try {
		for (ULong i = 0, cnt = (ULong)members_.size (); i < cnt; ++i) {
			const Member& m = members_ [i];
			if (!m.type->equivalent (other->member_type (i))
				|| m.visibility != other->member_visibility (i)) {
				ret = false;
				break;
			}
		}
	} catch (...) {
		g_ORB->type_code_pair_pop ();
		throw;
	}
	g_ORB->type_code_pair_pop ();
	return ret;
}

}
}
