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
#include "ExecDomain.h"
#include "Legacy/Process.h"

using namespace std;
using namespace Nirvana::Legacy::Core;

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
	ExecDomain::async_call (deadline, *this, g_core_free_sync_context, &g_shared_mem_context);
}

class RunAndShutdown :
	public Process
{
protected:
	RunAndShutdown (const vector <StringView>& argv, const vector <StringView>& envp, int& ret) :
		Process (argv.front (), argv, envp),
		ret_ (ret)
	{}

	virtual void run ()
	{
		Legacy::Core::Process::run ();
		ret_ = Legacy::Core::Process::ret ();
		Scheduler::shutdown ();
	}

private:
	int& ret_;
};

void Startup::run ()
{
	if (argc_ > 1) {
		std::vector <StringView> argv;
		argv.reserve (argc_ - 1);
		for (char** arg = argv_ + 1, **end = argv_ + argc_; arg != end; ++arg) {
			argv.emplace_back (*arg);
		}
		std::vector <StringView> envp;
		for (char** env = envp_; *env; ++env) {
			envp.emplace_back (*env);
		}
		CoreRef <RunAndShutdown> process = CoreRef <RunAndShutdown>::create
			<ImplDynamic <RunAndShutdown> > (ref (argv), ref (envp), ref (ret_));
		process->spawn ();
	}
}

void Startup::on_exception () NIRVANA_NOEXCEPT
{
	exception_ = std::current_exception ();
	Scheduler::shutdown ();
}

void Startup::on_crash (const siginfo_t& signal) NIRVANA_NOEXCEPT
{
	exception_code_ = CORBA::SystemException::EC_UNKNOWN;
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
