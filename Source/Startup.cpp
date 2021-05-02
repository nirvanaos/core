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

namespace Nirvana {
namespace Core {

class RunProcess : public Runnable
{
public:
	RunProcess (Startup& startup, int argc, char* argv [], char* envp [], int& ret) :
		startup_ (startup),
		executable_ (argv [0]),
		argc_ (argc),
		argv_ (argv),
		envp_ (envp)
	{}

	virtual void run ();
	virtual void on_exception () NIRVANA_NOEXCEPT;
	virtual void on_crash (int error_code) NIRVANA_NOEXCEPT;

private:
	Startup& startup_;
	Legacy::Core::Executable executable_;
	int argc_;
	char** argv_;
	char** envp_;
};

void RunProcess::run ()
{
	startup_.ret (executable_.startup ()->main (argc_, argv_, envp_));
}

void RunProcess::on_exception () NIRVANA_NOEXCEPT
{
	startup_.on_exception ();
}

void RunProcess::on_crash (int error_code) NIRVANA_NOEXCEPT
{
	startup_.on_crash (error_code);
}

void Startup::run ()
{
	initialize ();
}

void Startup::on_exception () NIRVANA_NOEXCEPT
{
	exception_ = std::current_exception ();
	Scheduler::shutdown ();
}

void Startup::on_crash (int error_code) NIRVANA_NOEXCEPT
{
	exception_code_ = (CORBA::SystemException::Code)error_code;
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
