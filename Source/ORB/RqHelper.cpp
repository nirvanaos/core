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
#include "RqHelper.h"
#include <Nirvana/signal_defs.h>

namespace CORBA {
namespace Core {

void RqHelper::check_align (size_t align)
{
	if (!(align == 1 || align == 2 || align == 4 || align == 8))
		throw BAD_PARAM ();
}

Any RqHelper::signal2exception (const siginfo& signal) noexcept
{
	Exception::Code ec ;
	if (signal.si_excode != Exception::EC_NO_EXCEPTION)
		ec = (Exception::Code)signal.si_excode;
	else
		ec = SystemException::EC_UNKNOWN;
	const Internal::ExceptionEntry* ee = SystemException::_get_exception_entry (ec);
	std::aligned_storage <sizeof (SystemException), alignof (SystemException)>::type buf;
	(ee->construct) (&buf);
	Any a;
	a <<= (const Exception&)buf;
	return a;
}

Any RqHelper::exception2any (Exception&& e)
{
	Any a;
	a <<= std::move (e);
	return a;
}

}
}
