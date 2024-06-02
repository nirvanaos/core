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
#include <Nirvana/Shell_s.h>
#include "EventSync.h"
#include "ExecDomain.h"
#include "NameService/FileSystem.h"
#include "Executable.h"
#include "ORB/Services.h"

namespace Nirvana {
namespace Core {

/// Nirvana process.
class Process :
	public CORBA::servant_traits <Nirvana::Process>::Servant <Process>,
	public Runnable
{
	typedef CORBA::servant_traits <Nirvana::Process>::Servant <Process> Servant;

public:
	/// \returns Current process if execution is in legacy mode.
	///          Otherwise returns `nullptr`.
	static Process* current_ptr () noexcept;

	static Process& current () noexcept
	{
		Process* p = current_ptr ();
		assert (p);
		return *p;
	}

	// Process::

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

	static Nirvana::Process::_ref_type spawn (AccessDirect::_ptr_type file,
		IDL::Sequence <IDL::String>& argv, const IDL::String& work_dir,
		const FileDescriptors& inherit, ProcessCallback::_ptr_type callback)
	{
		CORBA::servant_reference <Process> servant = CORBA::make_reference <Process> (
			std::ref (file), std::ref (argv), std::ref (work_dir), std::ref (inherit), callback);

		Nirvana::Process::_ref_type ret = servant->_this ();

		// Hold the proxy to keep object alive even if process is not referenced by any client.
		servant->proxy_ = ret;

		try {
			Nirvana::Core::ExecDomain::start_process (*servant, servant->sync_context (),
				Ref <MemContext> (servant->mem_context_));
		} catch (...) {
			servant->proxy_ = nullptr;
			throw;
		}

		return ret;
	}

private:
	template <class S, class ... Args> friend
		CORBA::servant_reference <S> CORBA::make_reference (Args&& ...);

	// Constructor. Do not call directly.
	Process (AccessDirect::_ptr_type file,
		IDL::Sequence <IDL::String>& argv, const IDL::String& work_dir,
		const FileDescriptors& inherit, ProcessCallback::_ptr_type callback) :
		state_ (INIT),
		ret_ (-1),
		executable_ (file),
		argv_ (std::move (argv)),
		callback_ (callback),
		mem_context_ (MemContext::create (MemContext::current ().heap ())),
		exec_domain_ (nullptr)
	{
		Ref <MemContext> tmp (mem_context_);
		ExecDomain& ed = ExecDomain::current ();
		ed.mem_context_swap (tmp);
		try {
			mem_context_->file_descriptors ().set_inherited_files (inherit);
			if (!work_dir.empty ())
				mem_context_->current_dir ().chdir (work_dir);
		} catch (...) {
			ed.mem_context_swap (tmp);
			throw;
		}
		ed.mem_context_swap (tmp);
	}

private:
	// Core::Runnable::
	virtual void run () override;
	virtual void on_crash (const siginfo& signal) noexcept override;

	// Must be called in run()

	void run_begin ()
	{
		exec_domain_ = &Nirvana::Core::ExecDomain::current ();
	}

	void run_end ()
	{
		exec_domain_ = nullptr;
	}

private:
	typedef std::vector <std::string> Strings;
	void finish () noexcept;
	void error_message (const char* msg) const;

private:
	enum
	{
		INIT,
		RUNNING,
		COMPLETED
	} state_;

	int ret_;
	ImplStatic <Executable> executable_;
	Strings argv_;
	ProcessCallback::_ref_type callback_;
	Nirvana::Process::_ref_type proxy_;
	Ref <MemContext> mem_context_;
	EventSync completed_;
	ExecDomain* exec_domain_;
};

}
}

#endif
