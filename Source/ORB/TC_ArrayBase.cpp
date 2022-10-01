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

TC_ArrayBase::TC_ArrayBase (TCKind kind, TC_Ref&& content_type, ULong bound) :
	TC_Base (kind),
	content_type_ (std::move (content_type)),
	element_size_ (content_type_->n_size ()),
	element_align_ (content_type_->n_align ()),
	bound_ (bound)
{
	TCKind element_kind = TCKind::tk_array == kind
		? get_array_kind (content_type_) : content_type_->kind ();
	switch (element_kind) {
		case TCKind::tk_char:
			content_kind_ = KIND_CHAR;
			break;
		case TCKind::tk_wchar:
			content_kind_ = KIND_WCHAR;
			break;
		default:
			if (content_type_->n_is_CDR ())
				content_kind_ = KIND_CDR;
			else
				content_kind_ = KIND_NONCDR;
	}
}

TCKind TC_ArrayBase::get_array_kind (TypeCode::_ptr_type tc)
{
	TCKind kind = tc->kind ();
	if (TCKind::tk_array == kind)
		return get_array_kind (tc->content_type ());
	else
		return kind;
}

}
}
