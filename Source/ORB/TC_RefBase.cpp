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
#include "TC_RefBase.h"

namespace CORBA {
namespace Core {

size_t TC_RefBase::_s_n_aligned_size (Internal::Bridge <TypeCode>*, Internal::Interface*)
{
	return sizeof (Internal::Interface*);
}

size_t TC_RefBase::_s_n_CDR_size (Internal::Bridge <TypeCode>*, Internal::Interface*)
{
	return sizeof (Internal::Interface*);
}

size_t TC_RefBase::_s_n_align (Internal::Bridge <TypeCode>*, Internal::Interface*)
{
	return alignof (Internal::Interface*);
}

Octet TC_RefBase::_s_n_is_CDR (Internal::Bridge <TypeCode>*, Internal::Interface*)
{
	return false;
}

}
}
