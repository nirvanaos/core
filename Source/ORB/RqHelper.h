/// \file
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
#ifndef NIRVANA_ORB_CORE_RQHELPER_H_
#define NIRVANA_ORB_CORE_RQHELPER_H_
#pragma once

#include <CORBA/CORBA.h>

struct siginfo;

namespace CORBA {
namespace Core {

class RqHelper
{
public:
	static Object::_ptr_type interface2object (Internal::Interface::_ptr_type itf)
	{
		return Internal::interface2object (itf);
	}

	static ValueBase::_ptr_type value_type2base (Internal::Interface::_ptr_type val)
	{
		return Internal::value_type2base (val);

	}

	static AbstractBase::_ptr_type abstract_interface2base (Internal::Interface::_ptr_type itf)
	{
		return Internal::abstract_interface2base (itf);
	}

	static void check_align (size_t align);
	static Any signal2exception (const siginfo& signal) noexcept;
	static Any exception2any (Exception&& e);

};

}
}

#endif
