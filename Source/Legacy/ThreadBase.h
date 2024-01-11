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
#ifndef NIRVANA_LEGACY_CORE_THREADBASE_H_
#define NIRVANA_LEGACY_CORE_THREADBASE_H_
#pragma once

#include "../RuntimeGlobal.h"
#include <Nirvana/SimpleList.h>

namespace Nirvana {
namespace Legacy {
namespace Core {

class Process;

/// Legacy thread base.
class NIRVANA_NOVTABLE ThreadBase :
	public Nirvana::Core::Runnable,
	public SimpleList <ThreadBase>::Element
{
public:
	/// \returns Current thread if execution is in legacy mode.
	///          Otherwise returns `nullptr`.
	static ThreadBase* current_ptr () noexcept;

	/// When we run in the legacy subsystem, every thread is a ThreadLegacy instance.
	static ThreadBase& current () noexcept
	{
		ThreadBase* p = current_ptr ();
		assert (p);
		return *p;
	}

	virtual Process& process () noexcept = 0;

	Nirvana::Core::RuntimeGlobal& runtime_global () noexcept
	{
		return runtime_global_;
	}

	Nirvana::Core::ExecDomain* exec_domain () const
	{
		return exec_domain_;
	}

protected:
	ThreadBase () :
		exec_domain_ (nullptr)
	{}

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
	Nirvana::Core::RuntimeGlobal runtime_global_;
	Nirvana::Core::ExecDomain* exec_domain_;
};

}
}
}

#endif
