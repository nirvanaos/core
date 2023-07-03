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
#include <CORBA/CosNaming.h>
#include <CORBA/I_var.h>
#include <Nirvana/Legacy/Legacy_Process_s.h>
#include "../Event.h"
#include "../MemContext.h"
#include "../RuntimeSupport.h"
#include "../MemContextObject.h"
#include "../Console.h"
#include "../NameService/FileSystem.h"
#include "Executable.h"
#include "ThreadBase.h"
#include "../ORB/Services.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Legacy process.
class Process :
	public CORBA::servant_traits <Nirvana::Legacy::Process>::Servant <Process>,
	public Nirvana::Core::MemContext,
	public ThreadBase
{
	typedef CORBA::servant_traits <Nirvana::Legacy::Process>::Servant <Process> Servant;

public:
	// Override ThreadBase
	using Servant::operator new;
	using Servant::operator delete;

	void _add_ref () noexcept override
	{
		Servant::_add_ref ();
	}

	void _remove_ref () noexcept override
	{
		Servant::_remove_ref ();
	}

	/// \returns Current process if execution is in legacy mode.
	///          Otherwise returns `nullptr`.
	static Process* current_ptr () noexcept;

	static Process& current () noexcept
	{
		Process* p = current_ptr ();
		assert (p);
		return *p;
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

	void wait () noexcept
	{
		completed_.wait ();
	}

	static bool kill ()
	{
		throw_NO_IMPLEMENT ();
	}

	Nirvana::Core::SyncContext& sync_context () noexcept
	{
		return executable_;
	}

	static IDL::String get_current_dir_name ();
	static Dir::_ref_type get_current_dir ();
	static void chdir (const IDL::String& path);

	~Process ();

	static Legacy::Process::_ref_type spawn (const IDL::String& file,
		IDL::Sequence <IDL::String>& argv, IDL::Sequence <IDL::String>& envp, IDL::String& work_dir,
		ProcessCallback::_ptr_type callback)
	{
		CORBA::servant_reference <Process> servant = CORBA::make_reference <Process> (
			std::ref (file), std::ref (argv), std::ref (envp), std::ref (work_dir), callback);
		servant->start ();

		Legacy::Process::_ref_type ret = servant->_this ();
		Nirvana::Core::ExecDomain::start_legacy_process (*servant);
		return ret;
	}

private:
	template <class S, class ... Args> friend
		CORBA::servant_reference <S> CORBA::make_reference (Args ...);

	// Constructor. Do not call directly.
	Process (const IDL::String& file,
		IDL::Sequence <IDL::String>& argv, IDL::Sequence <IDL::String>& envp, IDL::String& work_dir,
		ProcessCallback::_ptr_type callback) :
		// Inherit the parent heap
		MemContext (Nirvana::Core::ExecDomain::current ().mem_context ().heap ()),
		state_ (INIT),
		ret_ (-1),
		executable_ (std::ref (file)),
		argv_ (std::move (argv)),
		envp_ (std::move (envp)),
		current_dir_name_ (std::move (work_dir)),
		callback_ (callback),
		sync_domain_ (&Nirvana::Core::SyncDomain::current ())
	{
		Dir::_ref_type dir = Dir::_narrow (Nirvana::Core::FileSystem::resolve_absolute_path (current_dir_name_));
		if (!dir || dir->type () != Dir::FileType::directory)
			throw RuntimeError (ENOTDIR);
		current_dir_ = std::move (dir);
	}

private:
	// Core::Runnable::
	virtual void run () override;
	virtual void on_crash (const siginfo& signal) noexcept override;

	// Core::MemContext::
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj) override;
	virtual void runtime_proxy_remove (const void* obj) noexcept override;
	virtual void on_object_construct (Nirvana::Core::MemContextObject& obj)
		noexcept override;
	virtual void on_object_destruct (Nirvana::Core::MemContextObject& obj)
		noexcept override;

	// ThreadBase::
	virtual Process& process () noexcept override;

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

	virtual void on_thread_proc_end () noexcept override
	{
		// Release proxy reference on the main thread finish
		CORBA::Internal::interface_release (&Legacy::Process::_ptr_type (_this ()));
		Nirvana::Core::Scheduler::activity_end ();
	}

	void chdir_sync (IDL::String&& path);

private:
	typedef std::vector <std::string> Strings;
	typedef std::vector <char*, Nirvana::Core::UserAllocator <char*> > Pointers;
	static void copy_strings (Strings& src, Pointers& dst);
	void finish () noexcept;

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
	IDL::String current_dir_name_;
	Dir::_ref_type current_dir_;
	ProcessCallback::_ref_type callback_;
	Nirvana::Core::Ref <Nirvana::Core::SyncContext> sync_domain_;
	Nirvana::Core::Event completed_;
};

}
}
}

#endif
