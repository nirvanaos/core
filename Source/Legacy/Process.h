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
#include "../IDL/Legacy_Process_s.h"
#include "Executable.h"
#include "ExecDomain.h"
#include "../MemContext.h"
#include "../RuntimeSupport.h"
#include "../MemContextObject.h"
#include "Console.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

class ThreadLegacy;

/// Legacy process.
class Process :
	public CORBA::servant_traits <Nirvana::Legacy::Process>::Servant <Process>,
	public Nirvana::Core::LifeCyclePseudo <Process>,
	public Nirvana::Core::MemContext,
	public Nirvana::Core::Runnable
{
public:
	static Legacy::Process::_ref_type spawn (const std::string& file,
		const std::vector <std::string>& argv, const std::vector <std::string>& envp,
		ProcessCallback::_ptr_type callback);

	Nirvana::Core::SyncContext& free_sync_context ()
	{
		return executable_;
	}

	// Legacy::Process::

	bool completed () const
	{
		return 0 == thread_count_;
	}

	int exit_code () const
	{
		return ret_;
	}

	static void wait ()
	{
		throw_NO_IMPLEMENT ();
	}

	static bool kill ()
	{
		throw_NO_IMPLEMENT ();
	}

	// Thread callbacks

	void on_thread_start (ThreadLegacy& thread) NIRVANA_NOEXCEPT
	{
		if (!main_thread_)
			main_thread_ = &thread;

		thread_count_.increment ();
	}

	void on_thread_finish (ThreadLegacy& thread) NIRVANA_NOEXCEPT
	{
		if (0 == thread_count_.decrement ()) {
			object_list_.clear ();
			runtime_support_.clear ();
		}
	}

	Process (const std::string& file,
		const std::vector <std::string>& argv, const std::vector <std::string>& envp,
		ProcessCallback::_ptr_type callback) :
		executable_ (file),
		ret_ (-1),
		main_thread_ (nullptr),
		thread_count_ (0),
		callback_ (callback),
		sync_ (Nirvana::Core::SyncContext::current ())
	{
		copy_strings (argv, argv_);
		copy_strings (envp, envp_);
	}

	~Process ()
	{}

	void _add_ref () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::LifeCyclePseudo <Process>::_add_ref ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::LifeCyclePseudo <Process>::_remove_ref ();
	}

	Legacy::Process::_ref_type _this ()
	{
		if (proxy_)
			return proxy_;
		Legacy::Process::_ref_type proxy = 
			CORBA::servant_traits <Nirvana::Legacy::Process>::Servant <Process>::_this ();
		if (callback_)
			proxy_ = proxy;
		return proxy;
	}

private:

	virtual void run ();
	virtual void on_exception () NIRVANA_NOEXCEPT;
	virtual void on_crash (const siginfo_t& signal) NIRVANA_NOEXCEPT;

	// Process is shared object, so we must use shared allocators in constructor.
	typedef std::vector <Nirvana::Core::SharedString, 
		Nirvana::Core::SharedAllocator <Nirvana::Core::SharedString> > Strings;

	static void copy_strings (const std::vector <std::string>& src,
		Strings& dst);
	
	typedef std::vector <char*, Nirvana::Core::UserAllocator <char*> > Pointers;
	static void copy_strings (Strings& src, Pointers& dst);

	// MemContext methods

	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);
	virtual void runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT;
	virtual void on_object_construct (Nirvana::Core::MemContextObject& obj)
		NIRVANA_NOEXCEPT;
	virtual void on_object_destruct (Nirvana::Core::MemContextObject& obj)
		NIRVANA_NOEXCEPT;
	virtual Nirvana::Core::TLS& get_TLS () NIRVANA_NOEXCEPT;

private:
	Nirvana::Core::ImplStatic <Executable> executable_;
	int ret_;
	Nirvana::Core::RuntimeSupportImpl runtime_support_;
	Nirvana::Core::MemContextObjectList object_list_;
	Nirvana::Core::Console console_;
	Strings argv_, envp_;
	ThreadLegacy* main_thread_;
	Nirvana::Core::AtomicCounter <false> thread_count_;
	ProcessCallback::_ref_type callback_;
	Legacy::Process::_ref_type proxy_;
	Nirvana::Core::SyncContext& sync_; // Sync domain
};

}
}
}

#endif
