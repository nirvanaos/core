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
#include "TC_Sequence.h"

namespace CORBA {
namespace Core {

TC_Sequence::TC_Sequence (TC_Ref&& content_type, ULong bound) :
	Impl (TCKind::tk_sequence),
	content_type_ (std::move (content_type)),
	element_size_ (content_type_->n_size ()),
	element_align_ (content_type_->n_align ()),
	bound_ (bound)
{
	switch (content_type_->kind ()) {
		case TCKind::tk_char:
			kind_ = KIND_CHAR;
			break;
		case TCKind::tk_wchar:
			kind_ = KIND_WCHAR;
			break;
		default:
			if (content_type_->n_is_CDR ())
				kind_ = KIND_CDR;
			else
				kind_ = KIND_NONCDR;
	}
}

void TC_Sequence::marshal (const void* src, size_t count, Internal::IORequest_ptr rq, bool out) const
{
	Internal::check_pointer (src);
	ABI* psrc = (ABI*)src, *end = psrc + count;
	switch (kind_) {
		case KIND_CHAR:
			for (; psrc != end; ++psrc) {
				rq->marshal_char_seq ((IDL::Sequence <Char>&)*psrc, out);
			}
			break;
		case KIND_WCHAR:
			for (; psrc != end; ++psrc) {
				rq->marshal_wchar_seq ((IDL::Sequence <WChar>&)*psrc, out);
			}
			break;
		case KIND_CDR:
			for (; psrc != end; ++psrc) {
				size_t zero = 0;
				rq->marshal_seq (element_align_, element_size_, psrc->size, psrc->ptr, out ? psrc->allocated : zero);
			}
			break;
		default:
			for (; psrc != end; ++psrc) {
				size_t size = psrc->size;
				rq->marshal_seq_begin (size);
				if (size) {
					Octet* p = (Octet*)psrc->ptr;
					if (out)
						content_type_->n_marshal_out (p, size, rq);
					else
						content_type_->n_marshal_in (p, size, rq);
				}
			}
			break;
	}
}

}
}
