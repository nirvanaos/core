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
#include "remarshal_output.h"
#include <Port/config.h>

namespace CORBA {
using namespace Internal;
namespace Core {

static void remarshal_param (TypeCode::_ptr_type tc, std::vector <Octet>& buf,
	IORequest::_ptr_type src, IORequest::_ptr_type dst)
{
	size_t cb = tc->n_aligned_size ();
	if (buf.size () < cb) {
		std::vector <Octet> new_buf (Nirvana::round_up (cb, Nirvana::Core::HEAP_UNIT_DEFAULT));
		buf.swap (new_buf);
	}
	tc->n_construct (buf.data ());
	tc->n_unmarshal (src, 1, buf.data ());
	try {
		tc->n_marshal_out (buf.data (), 1, dst);
	} catch (...) {
		tc->n_destruct (buf.data ());
		throw;
	}
	tc->n_destruct (buf.data ());
}

void remarshal_output (const Operation& metadata, IORequest::_ptr_type src, IORequest::_ptr_type dst)
{
	std::vector <Octet> buf (Nirvana::Core::HEAP_UNIT_DEFAULT);
	if (metadata.return_type)
		remarshal_param ((metadata.return_type) (), buf, src, dst);
	for (const Parameter* param = metadata.output.p, *end = param + metadata.output.size;
		param != end; ++param) {
		remarshal_param ((param->type) (), buf, src, dst);
	}
	src->unmarshal_end ();
}

}
}
