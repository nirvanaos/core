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
#ifndef NIRVANA_ORB_CORE_SYSTEMEXCEPTIONHOLDER_H_
#define NIRVANA_ORB_CORE_SYSTEMEXCEPTIONHOLDER_H_
#pragma once

#include <CORBA/CORBA.h>

namespace CORBA {
namespace Core {

class SystemExceptionHolder
{
public:
	SystemExceptionHolder () noexcept
	{
		reset ();
	}

	void set_exception (const Exception& ex) noexcept;

	void set_exception (SystemException::Code ex_code) noexcept
	{
		ex_code_ = ex_code;
	}

	void check () const;

	void reset () noexcept;

private:
	SystemException::Code ex_code_;
	SystemException::_Data ex_data_;
};

}
}

#endif
