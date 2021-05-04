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
#include "Startup.h"
#include "Scheduler.h"
#include "Runnable.h"
#include "Legacy/Executable.h"
#include "Legacy/Process.h"

namespace Nirvana {
namespace Core {

Startup::Startup (int argc, char* argv [], char* envp []) :
	executable_ (nullptr),
	argc_ (argc),
	argv_ (argv),
	envp_ (envp),
	ret_ (0),
	exception_code_ (CORBA::Exception::EC_NO_EXCEPTION)
{}

void Startup::run ()
{
	if (!executable_) {
		initialize ();
		if (argc_ > 1) {
			executable_ = new Nirvana::Legacy::Core::Executable (argv_ [1]);
			Nirvana::Legacy::Core::Process::spawn (*this);
		}
	} else {
		ret_ = executable_->startup ()->main (argc_ - 1, argv_ + 1, envp_);
		Scheduler::shutdown ();
	}
}

void Startup::on_exception () NIRVANA_NOEXCEPT
{
	exception_ = std::current_exception ();
	delete executable_;
	executable_ = nullptr;
	Scheduler::shutdown ();
}

void Startup::on_crash (int error_code) NIRVANA_NOEXCEPT
{
	exception_code_ = (CORBA::SystemException::Code)error_code;
	delete executable_;
	executable_ = nullptr;
	Scheduler::shutdown ();
}

void Startup::check () const
{
	if (CORBA::Exception::EC_NO_EXCEPTION != exception_code_)
		CORBA::SystemException::_raise_by_code (exception_code_);
	else if (exception_)
		std::rethrow_exception (exception_);
}

}
}
