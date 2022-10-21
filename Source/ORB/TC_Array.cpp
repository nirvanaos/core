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
#include "TC_Array.h"

namespace CORBA {
namespace Core {

void TC_Array::marshal (const void* src, size_t count, Internal::IORequest_ptr rq, bool out) const
{
	Internal::check_pointer (src);
	switch (content_kind_) {
		case KIND_CHAR:
			rq->marshal_char (count * element_size_, (const Char*)src);
			break;
		case KIND_WCHAR:
			rq->marshal_wchar (count * element_size_ / sizeof (WChar), (const WChar*)src);
			break;
		case KIND_CDR:
			rq->marshal (element_align_, element_size_ * count, src);
			break;
		default:
			if (out)
				content_type_->n_marshal_out (const_cast <void*> (src), count, rq);
			else
				content_type_->n_marshal_in (src, count, rq);
	}
}

}
}
