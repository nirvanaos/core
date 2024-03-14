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
#include "TC_Sequence.h"

namespace CORBA {
namespace Core {

void TC_Sequence::marshal (const void* src, size_t count, Internal::IORequest_ptr rq, bool out) const
{
	Internal::check_pointer (src);
	ABI* psrc = (ABI*)src, *end = psrc + count;
	if (KIND_WCHAR == content_kind_) {
		for (; psrc != end; ++psrc) {
			rq->marshal_wchar_seq ((IDL::Sequence <WChar>&) * psrc, out);
		}
	} else {
		for (; psrc != end; ++psrc) {
			size_t size = psrc->size;
			rq->marshal_seq_begin (size);
			if (size) {
				if (out)
					content_type_->n_marshal_out (psrc->ptr, size, rq);
				else
					content_type_->n_marshal_in (psrc->ptr, size, rq);
			}
		}
	}
}

}
}
