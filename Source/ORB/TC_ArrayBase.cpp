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
#include "TC_ArrayBase.h"

namespace CORBA {
namespace Core {

bool TC_ArrayBase::set_content_type (TC_Ref&& content_type, ULong bound)
{
	content_type_ = std::move (content_type);
	bound_ = bound;
	return !content_type_.is_recursive ();
}

TCKind TC_ArrayBase::get_array_kind (TypeCode::_ptr_type tc)
{
	TypeCode::_ref_type t = dereference_alias (tc);
	TCKind kind = t->kind ();
	if (TCKind::tk_array == kind)
		return get_array_kind (t->content_type ());
	else
		return kind;
}

bool TC_ArrayBase::mark () noexcept
{
	if (!TC_ComplexBase::mark ())
		return false;
	content_type_.mark ();
	return true;
}

}
}
