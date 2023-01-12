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
#include <Nirvana/Legacy/Legacy_Process_s.h>
#include "../ExecDomain.h"
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
public:
	static Legacy::Process::_ref_type spawn (const std::string& file,
		std::vector <std::string>& argv, std::vector <std::string>& envp,
		ProcessCallback::_ptr_type callback)
	{
		// Cheat ObjectFactory
		Nirvana::Core::TLS& tls = Nirvana::Core::TLS::current ();
		CORBA::Internal::ObjectFactory::StatelessCreationFrame scf (nullptr, 1, 0,
			tls.get (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY));
		tls.set (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY, &scf);

		CORBA::servant_reference <Process> servant;
		try {
			servant = CORBA::make_reference <Process> (
				std::ref (file), std::ref (argv), std::ref (envp), callback);
		} catch (...) {
			tls.set (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY, scf.next ());
			throw;
		}
		tls.set (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY, scf.next ());

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

	// Constructor. Do not call directly.
	Process (const std::string& file,
		std::vector <std::string>& argv, std::vector <std::string>& envp,
		ProcessCallback::_ptr_type callback) :
		ThreadBase (heap ()),
		state_ (INIT),
		ret_ (-1),
		executable_ (std::ref (file)),
		argv_ (std::move (argv)),
		envp_ (std::move (envp)),
		callback_ (callback)
	{
		assert (Nirvana::Core::SyncContext::current ().is_free_sync_context ());
	}

	~Process ()
	{}

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

	Nirvana::Core::SyncContext& sync_context () NIRVANA_NOEXCEPT
	{
		return executable_;
	}

	using Nirvana::Core::SharedObject::operator new;
	using Nirvana::Core::SharedObject::operator delete;

	void _add_ref () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::LifeCyclePseudo <Process>::_add_ref ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::LifeCyclePseudo <Process>::_remove_ref ();
	}

private:

	// Core::Runnable::

	virtual void run ();
	virtual void on_exception () NIRVANA_NOEXCEPT;
	virtual void on_crash (const siginfo& signal) NIRVANA_NOEXCEPT;

	// Core::MemContext::

	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);
	virtual void runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT;
	virtual void on_object_construct (Nirvana::Core::MemContextObject& obj)
		NIRVANA_NOEXCEPT;
	virtual void on_object_destruct (Nirvana::Core::MemContextObject& obj)
		NIRVANA_NOEXCEPT;
	virtual Nirvana::Core::TLS& get_TLS () NIRVANA_NOEXCEPT;

	// ThreadBase::

	virtual Process& process () NIRVANA_NOEXCEPT;

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
	Legacy::Process::_ref_type proxy_;

	// Synchronizer
	Nirvana::Core::StaticallyAllocated <Nirvana::Core::ImplStatic <
		Nirvana::Core::SyncDomainImpl> > sync_;
};

}
}
}

#endif
