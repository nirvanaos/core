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
#ifndef NIRVANA_LEGACY_CORE_PROCESS_H_
#define NIRVANA_LEGACY_CORE_PROCESS_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/I_var.h>
#include <Nirvana/Legacy/Legacy_Process_s.h>
#include "../Event.h"
#include "../MemContext.h"
#include "../RuntimeSupport.h"
#include "../MemContextObject.h"
#include "../Console.h"
#include "Executable.h"
#include "ThreadBase.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Legacy process.
class Process :
	public CORBA::servant_traits <Nirvana::Legacy::Process>::Servant <Process>,
	public Nirvana::Core::LifeCyclePseudo <Process>,
	public Nirvana::Core::MemContext,
	public ThreadBase
{
	typedef CORBA::servant_traits <Nirvana::Legacy::Process>::Servant <Process> Servant;

public:
	using Servant::operator new;
	using Servant::operator delete;

	static Legacy::Process::_ref_type spawn (const std::string& file,
		std::vector <std::string>& argv, std::vector <std::string>& envp,
		ProcessCallback::_ptr_type callback)
	{
		CORBA::servant_reference <Process> servant = CORBA::make_reference <Process> (
				std::ref (file), std::ref (argv), std::ref (envp), callback);
		servant->start ();

		Legacy::Process::_ref_type ret = servant->_this ();
		Nirvana::Core::ExecDomain::start_legacy_process (*servant);
		return ret;
	}

	// Legacy::Process::

	bool completed () const
	{
		return COMPLETED == state_;
	}

	int exit_code () const
	{
		if (COMPLETED != state_)
			throw_BAD_INV_ORDER ();
		return ret_;
	}

	void wait () NIRVANA_NOEXCEPT
	{
		completed_.wait ();
	}

	static bool kill ()
	{
		throw_NO_IMPLEMENT ();
	}

	// Constructor. Do not call directly.
	Process (const std::string& file,
		std::vector <std::string>& argv, std::vector <std::string>& envp,
		ProcessCallback::_ptr_type callback) :
		// Inherit the parent heap
		MemContext (Nirvana::Core::ExecDomain::current ().mem_context ().heap ()),
		state_ (INIT),
		ret_ (-1),
		executable_ (std::ref (file)),
		argv_ (std::move (argv)),
		envp_ (std::move (envp)),
		callback_ (callback),
		sync_domain_ (&Nirvana::Core::SyncDomain::current ())
	{}

	~Process ()
	{}

	Nirvana::Core::SyncContext& sync_context () NIRVANA_NOEXCEPT
	{
		return executable_;
	}

	void _add_ref () NIRVANA_NOEXCEPT override
	{
		Nirvana::Core::LifeCyclePseudo <Process>::_add_ref ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT override
	{
		Nirvana::Core::LifeCyclePseudo <Process>::_remove_ref ();
	}

private:
	// Core::Runnable::
	virtual void run () override;
	virtual void on_crash (const siginfo& signal) NIRVANA_NOEXCEPT override;

	// Core::MemContext::
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj) override;
	virtual void runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT override;
	virtual void on_object_construct (Nirvana::Core::MemContextObject& obj)
		NIRVANA_NOEXCEPT override;
	virtual void on_object_destruct (Nirvana::Core::MemContextObject& obj)
		NIRVANA_NOEXCEPT override;
	virtual Nirvana::Core::TLS& get_TLS () NIRVANA_NOEXCEPT override;

	// ThreadBase::
	virtual Process& process () NIRVANA_NOEXCEPT override;

	void start ()
	{
		CORBA::Internal::I_var <Legacy::Process> proxy (_this ());
		Nirvana::Core::Scheduler::activity_begin ();
		try {
			Nirvana::Core::Port::ThreadBackground::start ();
		} catch (...) {
			Nirvana::Core::Scheduler::activity_end ();
			throw;
		}
		proxy._retn (); // Keep proxy reference on successfull start
	}

	virtual void on_thread_proc_end () NIRVANA_NOEXCEPT override
	{
		// Release proxy reference on the main thread finish
		CORBA::Internal::interface_release (&Legacy::Process::_ptr_type (_this ()));
		Nirvana::Core::Scheduler::activity_end ();
	}

private:
	typedef std::vector <std::string> Strings;
	typedef std::vector <char*, Nirvana::Core::UserAllocator <char*> > Pointers;
	static void copy_strings (Strings& src, Pointers& dst);
	void finish () NIRVANA_NOEXCEPT;

private:
	enum
	{
		INIT,
		RUNNING,
		COMPLETED
	} state_;

	int ret_;
	Nirvana::Core::ImplStatic <Executable> executable_;
	Nirvana::Core::RuntimeSupportImpl runtime_support_;
	Nirvana::Core::MemContextObjectList object_list_;
	Nirvana::Core::Console console_;
	Strings argv_, envp_;
	ProcessCallback::_ref_type callback_;
	Nirvana::Core::Ref <Nirvana::Core::SyncContext> sync_domain_;
	Nirvana::Core::Event completed_;
};

}
}
}

#endif
