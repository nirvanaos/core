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

#include "../Thread.h"
#include "../SyncContext.h"
#include <Port/ThreadBackground.h>
#include "RuntimeSupportLegacy.h"

namespace Nirvana {

namespace Core {
class RuntimeSupport;
}

namespace Legacy {
namespace Core {

/// Background thread.
/// Used in the legacy mode implementation.
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
	virtual void schedule_call (Nirvana::Core::SyncDomain* sync_domain);

	/// Return execution domain to this context.
	virtual void schedule_return (Nirvana::Core::ExecDomain& exec_domain) NIRVANA_NOEXCEPT;

	/// Returns `nullptr` if it is free sync context.
	/// May be used for proxy optimization.
	/// When we marshal `in` parameters from free context we haven't to copy data.
	virtual Nirvana::Core::SyncDomain* sync_domain () NIRVANA_NOEXCEPT;

	/// Returns heap reference.
	// virtual Nirvana::Core::Heap& memory () = 0;

	RuntimeSupportLegacy& runtime_support () const NIRVANA_NOEXCEPT
	{
		assert (runtime_support_);
		return static_cast <RuntimeSupportLegacy&> (*runtime_support_);
	}

	virtual void yield () NIRVANA_NOEXCEPT;

protected:
	ThreadBackground (bool process);

	void start (RuntimeSupportLegacy& runtime_support, Nirvana::Core::Runnable& runnable);

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
