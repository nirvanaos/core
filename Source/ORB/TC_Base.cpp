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
#include "TC_Base.h"

namespace CORBA {
namespace Core {

TC_Base::String::String (const IDL::String& s) :
	Base (s.data (), s.size ())
{}

TC_Base::String& TC_Base::String::operator = (const IDL::String& s)
{
	assign (s.data (), s.size ());
	return *this;
}

bool TC_Base::String::operator == (const IDL::String& s) const NIRVANA_NOEXCEPT
{
	size_t cc = size ();
	if (cc == s.size ()) {
		const Char* p = data ();
		return std::equal (p, p + cc, s.data ());
	}
	return false;
}

TC_Base::String::operator IDL::String () const
{
	return IDL::String (data (), size ());
}

}
}
