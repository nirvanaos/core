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
#include "Nirvana_Process.h"
#include "Synchronized.h"
#include <iostream>

namespace Nirvana {
namespace Core {

Process* Process::current_ptr () noexcept
{
	const Nirvana::Core::ExecDomain& ed = Nirvana::Core::ExecDomain::current ();
	if (ed.sync_context ().is_process ())
		return static_cast <Process*> (ed.runnable ());
	else
		return nullptr;
}

Process::~Process ()
{}

void Process::run ()
{
	assert (INIT == state_);
	state_ = RUNNING;

	try {
		ret_ = executable_.main (argv_);
		executable_.execute_atexit ();
		executable_.cleanup ();
	} catch (const std::exception& ex) {
		ret_ = -1;
		std::string msg ("Unhandled exception: ");
		msg += ex.what ();
		msg += '\n';
		error_message (msg.c_str ());
	} catch (...) {
		ret_ = -1;
		error_message ("Unknown exception\n");
	}

	finish ();
}

void Process::finish () noexcept
{
	executable_.unload ();

	{
		Strings tmp (std::move (argv_));
	}
	state_ = COMPLETED;
	completed_.signal ();

	Nirvana::Process::_ref_type proxy (std::move (proxy_));

	if (callback_) {
		ProcessCallback::_ref_type callback (std::move (callback_));
		try {
			callback->on_process_finish (proxy);
		} catch (...) {
		}
	}
}

void Process::on_crash (const siginfo& signal) noexcept
{
	if (SIGABRT == signal.si_signo)
		ret_ = 3;
	else
		ret_ = -1;

	// Memory context must be unwound in ExecDomain::on_crash ()
	assert (ExecDomain::current ().mem_context_ptr () == mem_context_);
	
	// We are in our memory context but not in sync domain here.
	// I think, it is not urgent because nobody other can access process data here.

	error_message ("Process crashed.\n");
	finish ();
}

void Process::error_message (const char* msg) const
{
	mem_context_->file_descriptors ().write (2, msg, strlen (msg));
}

}
}
