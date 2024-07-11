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

#include "pch.h"
#include <CORBA/Server.h>
#include "Startup.h"
#include "ExecDomain.h"
#include "ProtDomain.h"
#include "initterm.h"
#include <Nirvana/Shell_s.h>
#include <Nirvana/File.h>
#include "ORB/Services.h"
#include <NameService/FileSystem.h>
#include "open_binary.h"

namespace Nirvana {
namespace Core {

Startup::Startup (int argc, char* argv [], char* envp []) :
	argc_ (argc),
	argv_ (argv),
	envp_ (envp),
	ret_ (0)
{}

void Startup::launch (DeadlineTime deadline)
{
	ExecDomain::async_call (deadline, *this, g_core_free_sync_context, &Heap::shared_heap ());
}

void Startup::run ()
{
	if (argc_ > 1) {

		bool cmdlet = false;
		const char* prog = argv_ [1];
		char** args = argv_ + 2, ** end = argv_ + argc_;

		if (prog [0] == '-' && prog [1] == 'c' && prog [2] == 0 && args != end) {
			prog = *(args++);
			cmdlet = true;
		}
		
		std::vector <std::string> argv;
		argv.reserve (end - args);
		for (char** arg = args; arg != end; ++arg) {
			argv.emplace_back (*arg);
		}

		Nirvana::File::_ref_type console = Nirvana::File::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::Console));
		assert (console);
		Nirvana::Access::_ref_type console_access = console->open (O_WRONLY, 0);
		/*
		try {
			console_access = console->open (O_RDWR, 0);
		} catch (...) {}*/
		FileDescriptors files;
		files.emplace_back (console_access, IDL::Sequence <uint16_t> ({ 1, 2 }));

		if (cmdlet) {
			ret_ = (int)the_shell->cmdlet (prog, argv, "/sbin", files);
		} else {

			IDL::String exe_path = prog;
			IDL::String translated;
			if (FileSystem::translate_path (exe_path, translated))
				exe_path = std::move (translated);

			if (!FileSystem::is_absolute (exe_path))
				exe_path = ProtDomain::binary_dir () + '/' + exe_path;
			auto ns = CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::NameService));
			AccessDirect::_ref_type binary = open_binary (ns, exe_path);

			ret_ = (int)the_shell->process (binary, argv, "/sbin", files);
		}
		Scheduler::shutdown (0);
	}
}

bool Startup::initialize () noexcept
{
	try {
		Nirvana::Core::initialize ();
	} catch (const CORBA::Exception& ex) {
		on_exception (ex);
		return false;
	}
	return true;
}

void Startup::on_exception (const CORBA::Exception& ex) noexcept
{
	exception_.set_exception (ex);
	Scheduler::shutdown (0);
}

void Startup::on_crash (const siginfo& signal) noexcept
{
	if (signal.si_excode == CORBA::Exception::EC_NO_EXCEPTION)
		exception_.set_exception (CORBA::SystemException::EC_UNKNOWN);
	else
		exception_.set_exception ((CORBA::SystemException::Code)signal.si_excode);
	Scheduler::shutdown (0);
}

}
}
