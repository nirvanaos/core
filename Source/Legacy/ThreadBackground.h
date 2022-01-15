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
#ifndef NIRVANA_LEGACY_CORE_THREADBACKGROUND_H_
#define NIRVANA_LEGACY_CORE_THREADBACKGROUND_H_
#pragma once

#include "../Thread.h"
#include "../SyncContext.h"
#include <Port/ThreadBackground.h>
#include "MemContextProcess.h"

namespace Nirvana {

namespace Core {
class RuntimeSupport;
}

namespace Legacy {
namespace Core {

/// Background thread.
class NIRVANA_NOVTABLE ThreadBackground :
	protected Nirvana::Core::Port::ThreadBackground,
	public Nirvana::Core::SyncContext
{
	friend class Nirvana::Core::Port::ThreadBackground;
public:
	/// Implementation - specific methods can be called explicitly.
	Nirvana::Core::Port::ThreadBackground& port ()
	{
		return *this;
	}

	/// When we run in the legacy subsystem, every thread is a ThreadBackground instance.
	static ThreadBackground& current () NIRVANA_NOEXCEPT
	{
		return static_cast <ThreadBackground&> (Nirvana::Core::Thread::current ());
	}

	/// Leave this context and enter to the synchronization domain.
	virtual void schedule_call (Nirvana::Core::SyncContext& target, Nirvana::Core::MemContext* mem_context);

	/// Return execution domain to this context.
	virtual void schedule_return (Nirvana::Core::ExecDomain& exec_domain) NIRVANA_NOEXCEPT;

	virtual void yield () NIRVANA_NOEXCEPT;

protected:
	ThreadBackground (bool process) :
		Nirvana::Core::Port::ThreadBackground (process)
	{}

	void start (Nirvana::Core::Runnable& runnable, Nirvana::Core::MemContext& mem_context);

private:
	void on_thread_proc_end ()
	{
		_remove_ref ();
	}
};

}
}
}

#endif
