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
#include "SystemExceptionHolder.h"

namespace CORBA {
namespace Core {

SystemExceptionHolder::SystemExceptionHolder () :
	ex_code_ (Exception::EC_NO_EXCEPTION),
	ex_data_ {0, CompletionStatus::COMPLETED_NO}
{}

void SystemExceptionHolder::set_exception (const Exception& ex) noexcept
{
	const SystemException* se = SystemException::_downcast (&ex);
	if (se) {
		ex_code_ = se->__code ();
		ex_data_ = se->_data ();
	} else
		ex_code_ = SystemException::EC_UNKNOWN;
}

void SystemExceptionHolder::check () const
{
	if (ex_code_ != Exception::EC_NO_EXCEPTION)
		SystemException::_raise_by_code (ex_code_, ex_data_.minor, ex_data_.completed);
}


}
}
