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

#include "Thread.h"
#include <CORBA/CosNaming.h>
#include <CORBA/I_var.h>
#include <Nirvana/Legacy/Legacy_Process_s.h>
#include "../EventSync.h"
#include "../MemContextUser.h"
#include "../NameService/FileSystem.h"
#include "Executable.h"
#include "../ORB/Services.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Legacy process.
class Process :
	public CORBA::servant_traits <Nirvana::Legacy::Process>::Servant <Process>,
	public Nirvana::Core::MemContextUser,
	public ThreadBase
{
	typedef CORBA::servant_traits <Nirvana::Legacy::Process>::Servant <Process> Servant;

public:
	// Override new/delete
	using Servant::operator new;
	using Servant::operator delete;

	// Override MemContext ref counting

	void _add_ref () noexcept
	{
		Servant::_add_ref ();
	}

	void _remove_ref () noexcept
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

	~Process ();

	static Legacy::Process::_ref_type spawn (AccessDirect::_ptr_type file,
		IDL::Sequence <IDL::String>& argv, IDL::Sequence <IDL::String>& envp, const IDL::String& work_dir,
		const InheritedFiles& inherit, ProcessCallback::_ptr_type callback)
	{
		CORBA::servant_reference <Process> servant = CORBA::make_reference <Process> (
			std::ref (file), std::ref (argv), std::ref (envp), std::ref (work_dir), std::ref (inherit), callback);

		Legacy::Process::_ref_type ret = servant->_this ();

		if (servant->callback_)
			servant->proxy_ = ret;

		try {
			Nirvana::Core::ExecDomain::async_call (INFINITE_DEADLINE, *servant, servant->sync_context (), servant);
		} catch (...) {
			servant->proxy_ = nullptr;
			throw;
		}

		return ret;
	}

	// Core::MemContextUser::
	virtual Nirvana::Core::RuntimeGlobal& runtime_global () noexcept override;
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj) override;
	virtual void runtime_proxy_remove (const void* obj) noexcept override;
	virtual void on_object_construct (Nirvana::Core::MemContextObject& obj)
		noexcept override;
	virtual void on_object_destruct (Nirvana::Core::MemContextObject& obj)
		noexcept override;
	virtual CosNaming::Name get_current_dir_name () const override;
	virtual void chdir (const IDL::String& path) override;
	virtual unsigned fd_add (Access::_ptr_type access) override;
	virtual void close (unsigned fd) override;
	virtual size_t read (unsigned fd, void* p, size_t size) override;
	virtual void write (unsigned fd, const void* p, size_t size) override;
	virtual uint64_t seek (unsigned fd, const int64_t& off, unsigned method) override;
	virtual unsigned fcntl (unsigned fd, int cmd, unsigned arg) override;
	virtual void flush (unsigned fd) override;
	virtual void dup2 (unsigned src, unsigned dst) override;
	virtual bool isatty (unsigned fd) override;
	virtual void push_back (unsigned fd, int c) override;
	virtual bool ferror (unsigned fd) override;
	virtual bool feof (unsigned fd) override;
	virtual void clearerr (unsigned fd) override;

	Legacy::Thread::_ref_type create_thread (Legacy::Runnable::_ptr_type runnable)
	{
		CORBA::servant_reference <Legacy::Core::Thread> thread = CORBA::make_reference <Legacy::Core::Thread> (
			std::ref (*this), runnable);
		
		// We must not destruct memory context while some thread exists
		_add_ref ();

		SYNC_BEGIN (*sync_domain_, nullptr);
		threads_.push_back (*thread);
		SYNC_END ();

		Nirvana::Core::ExecDomain::async_call (INFINITE_DEADLINE, *thread, sync_context (), this);

		return thread->_get_ptr ();
	}

	void delete_thread (Legacy::Core::Thread& thread)
	{
		SYNC_BEGIN (*sync_domain_, nullptr);
		static_cast <SimpleList <Legacy::Core::Thread>::Element> (thread).remove ();
		SYNC_END ();
		thread.~Thread ();
		heap ().release (&thread, sizeof (Legacy::Core::Thread));
		_remove_ref ();
	}

	void on_thread_crash (Legacy::Core::Thread& thread, const siginfo& signal)
	{
		// TODO: Implement
		unrecoverable_error (signal.si_signo);
	}

private:
	template <class S, class ... Args> friend
		CORBA::servant_reference <S> CORBA::make_reference (Args ...);

	// Constructor. Do not call directly.
	Process (AccessDirect::_ptr_type file,
		IDL::Sequence <IDL::String>& argv, IDL::Sequence <IDL::String>& envp, const IDL::String& work_dir,
		const InheritedFiles& inherit, ProcessCallback::_ptr_type callback) :
		// Inherit the parent heap
		MemContextUser (Nirvana::Core::ExecDomain::current ().mem_context ().heap (), inherit),
		state_ (INIT),
		ret_ (-1),
		executable_ (file),
		argv_ (std::move (argv)),
		envp_ (std::move (envp)),
		callback_ (callback),
		sync_domain_ (&Nirvana::Core::SyncDomain::current ())
	{
		if (!work_dir.empty ()) 
			MemContextUser::chdir (work_dir);
	}

private:
	// Core::Runnable::
	virtual void run () override;

public:
	virtual void on_crash (const siginfo& signal) noexcept override;

private:
	// ThreadBase::
	virtual Process& process () noexcept override;

private:
	typedef std::vector <std::string> Strings;
	typedef std::vector <char*, Nirvana::Core::UserAllocator <char*> > Pointers;
	static void copy_strings (Strings& src, Pointers& dst);
	void finish () noexcept;
	void error_message (const char* msg);

private:
	enum
	{
		INIT,
		RUNNING,
		COMPLETED
	} state_;

	int ret_;
	Nirvana::Core::ImplStatic <Executable> executable_;
	Strings argv_, envp_;
	Dir::_ref_type current_dir_;
	ProcessCallback::_ref_type callback_;
	Legacy::Process::_ref_type proxy_;
	Nirvana::Core::Ref <Nirvana::Core::SyncContext> sync_domain_;
	Nirvana::Core::EventSync completed_;
	SimpleList <Legacy::Core::Thread> threads_;
};

}
}
}

#endif
