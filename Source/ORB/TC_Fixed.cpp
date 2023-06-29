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

#include "TC_Fixed.h"

namespace CORBA {

using namespace Internal;

namespace Core {

void TC_Fixed::n_copy (void* dst, const void* src) const
{
	size_t cb = size ();
	check_pointer (dst);
	check_pointer (src);
	BCD_check ((const Octet*)src, cb);
	Nirvana::Core::virtual_copy (src, cb, dst);
}

void TC_Fixed::n_marshal_in (const void* src, size_t count, IORequest_ptr rq) const
{
	check_pointer (src);
	size_t cb = size ();
	size_t total = cb * count;
	for (const Octet* p = (const Octet*)src, *end = p + total; p != end; p += cb) {
		BCD_check (p, cb);
	}
	rq->marshal (1, total, src);
}

}
}
