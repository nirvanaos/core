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

// Using the precompiled header with CLang in this file causes error:
// The OLF static symbols not exported/imported.

#include <CORBA/Server.h>
#include "Startup.h"
#include "ExecDomain.h"
#include "initterm.h"
#include <Nirvana/Launcher.h>
#include <Nirvana/Legacy/Legacy_Process_s.h>
#include <Nirvana/File.h>
#include "ORB/Services.h"
#include <fnctl.h>

using namespace Nirvana::Legacy;

namespace Nirvana {
namespace Core {

Startup::Startup (int argc, char* argv [], char* envp []) :
	argc_ (argc),
	argv_ (argv),
	envp_ (envp),
	ret_ (0),
	exception_code_ (CORBA::Exception::EC_NO_EXCEPTION)
{}

void Startup::launch (DeadlineTime deadline)
{
	ExecDomain::async_call (deadline, this, g_core_free_sync_context, &Heap::shared_heap ());
}

class ShutdownCallback :
	public CORBA::servant_traits <ProcessCallback>::Servant <ShutdownCallback>
{
public:
	ShutdownCallback (int& ret) :
		ret_ (ret)
	{}

	void on_process_finish (Process::_ptr_type process) const
	{
		ret_ = process->exit_code ();
		Scheduler::shutdown ();
	}

private:
	int& ret_;
};

void Startup::run ()
{
	if (argc_ > 1) {
		
		std::vector <std::string> argv;
		argv.reserve (argc_ - 1);
		for (char** arg = argv_ + 1, **end = argv_ + argc_; arg != end; ++arg) {
			argv.emplace_back (*arg);
		}
		std::vector <std::string> envp;
		for (char** env = envp_; *env; ++env) {
			envp.emplace_back (*env);
		}

		ProcessCallback::_ref_type callback = CORBA::make_stateless <ShutdownCallback> (std::ref (ret_))->_this ();

		Nirvana::File::_ref_type console = Nirvana::File::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::Console));
		assert (console);
		Nirvana::Access::_ref_type console_access = console->open (O_WRONLY, 0);
		/*
		try {
			console_access = console->open (O_RDWR, 0);
		} catch (...) {}*/
		InheritedFiles files;
		files.emplace_back (console_access, IDL::Sequence <uint16_t> ({ 1, 2 }));

		Static <Launcher>::ptr ()->spawn (argv [0], argv, envp, "/sbin", files, callback);
	}
}

bool Startup::initialize () noexcept
{
	try {
		Nirvana::Core::initialize ();
	} catch (...) {
		exception_ = std::current_exception ();
		Port::Scheduler::shutdown ();
		return false;
	}
	return true;
}

void Startup::on_exception () noexcept
{
	exception_ = std::current_exception ();
	Scheduler::shutdown ();
}

void Startup::on_crash (const siginfo& signal) noexcept
{
	if (signal.si_excode == CORBA::Exception::EC_NO_EXCEPTION)
		exception_code_ = CORBA::SystemException::EC_UNKNOWN;
	else
		exception_code_ = (CORBA::Exception::Code)signal.si_excode;
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
