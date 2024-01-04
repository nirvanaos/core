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
#ifndef NIRVANA_LEGACY_CORE_THREAD_H_
#define NIRVANA_LEGACY_CORE_THREAD_H_
#pragma once

#include "Process.h"
#include "../Event.h"
#include <Nirvana/Legacy/Legacy_s.h>
#include <Nirvana/Legacy/Runnable.h>

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Legacy thread
class Thread :
	public CORBA::servant_traits <Nirvana::Legacy::Thread>::Servant <Thread>,
	public Nirvana::Core::LifeCyclePseudo <Thread>,
	public ThreadBase,
	public Nirvana::Core::MemContextObject
{
	typedef Nirvana::Core::LifeCyclePseudo <Thread> LifeCycle;

public:
	using Nirvana::Core::UserObject::operator new;
	using Nirvana::Core::UserObject::operator delete;

	void _add_ref () noexcept override
	{
		LifeCycle::_add_ref ();
	}

	void _remove_ref () noexcept override
	{
		LifeCycle::_remove_ref ();
	}

	Thread (Process& process, Nirvana::Legacy::Runnable::_ptr_type runnable) :
		process_ (&process),
		runnable_ (runnable)
	{
		if (!runnable)
			throw_BAD_PARAM ();
	}

	static Legacy::Thread::_ref_type spawn (Process& process, Nirvana::Legacy::Runnable::_ptr_type runnable)
	{
		CORBA::servant_reference <Thread> servant = CORBA::make_reference <Thread> (
			std::ref (process), runnable);
		servant->start ();
		try {
			Nirvana::Core::ExecDomain::start_legacy_thread (process, *servant);
		} catch (...) {
			servant->finish ();
			throw;
		}
		return servant->_get_ptr ();
	}

	void join ()
	{
		event_.wait ();
	}

private:
	// Core::Runnable::
	virtual void run () override;
	virtual void on_crash (const siginfo& signal) noexcept override;

	// ThreadBase::
	virtual Process& process () noexcept override
	{
		return *process_;
	}

private:
	Nirvana::Core::Ref <Process> process_;
	Nirvana::Legacy::Runnable::_ref_type runnable_;
	Nirvana::Core::Event event_;
};

}
}
}

#endif
