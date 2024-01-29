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

void Process::copy_strings (Strings& src, Pointers& dst)
{
	for (auto& s : src) {
		dst.push_back (const_cast <char*> (s.data ()));
	}
	dst.push_back (nullptr);
}

void Process::run ()
{
	run_begin ();

	assert (INIT == state_);
	state_ = RUNNING;

	try {
		Pointers v;
		v.reserve (argv_.size () + envp_.size () + 2);
		copy_strings (argv_, v);
		copy_strings (envp_, v);
		ret_ = executable_.main ((int)argv_.size (), v.data (), v.data () + argv_.size () + 1);
		executable_.execute_atexit (heap ());
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

	run_end ();
}

void Process::finish () noexcept
{
	MemContextUser::clear ();
	{
		Strings tmp (std::move (argv_));
	}
	{
		Strings tmp (std::move (envp_));
	}
	executable_.unbind ();
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
	error_message ("Process crashed.\n");
	finish ();
}

void Process::runtime_proxy_remove (const void* obj) noexcept
{
	assert (&MemContext::current () == this);

	// Debug iterators
#ifdef NIRVANA_DEBUG_ITERATORS
	if (RUNNING != state_)
		return;
#endif

	MemContextUser::runtime_proxy_remove (obj);
}

void Process::error_message (const char* msg)
{
	write (2, msg, strlen (msg));
}

}
}
