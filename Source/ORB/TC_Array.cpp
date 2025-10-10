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

void TC_Array::initialize ()
{
	traits_.element_count = bound_;
	get_array_traits (content_type_, traits_);
	element_align_ = traits_.element_type->n_align ();
	element_size_ = traits_.element_type->n_size ();

	SizeAndAlign sa (1);
	var_len_ = is_var_len (traits_.element_type, sa);
}

bool TC_Array::set_recursive (const IDL::String& id, const TC_Ref& ref) noexcept
{
	if (content_type_.replace_recursive_placeholder (id, ref))
		initialize ();
	return false;
}

}
}
