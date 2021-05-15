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
#ifndef NIRVANA_CORE_THREAD_H_
#define NIRVANA_CORE_THREAD_H_

#include "ExecContext.h"
#include <Port/Thread.h>

namespace Nirvana {
namespace Core {

class ExecDomain;
class SyncDomain;
class SyncContext;
class RuntimeSupportImpl;

/// Thread class.
class NIRVANA_NOVTABLE Thread :
	protected Port::Thread
{
	friend class Port::Thread;
public:
	// Implementation - specific methods can be called explicitly.
	Port::Thread& port () NIRVANA_NOEXCEPT
	{
		return *this;
	}

	/// Returns current thread.
	static Thread& current () NIRVANA_NOEXCEPT
	{
		Thread* p = Port::Thread::current ();
		assert (p);
		return *p;
	}

	/// Returns current thread.
	/// May return nullptr.
	static Thread* current_ptr () NIRVANA_NOEXCEPT
	{
		return Port::Thread::current ();
	}

	ExecDomain* exec_domain () const NIRVANA_NOEXCEPT
	{
		return exec_domain_;
	}

	void exec_domain (ExecDomain& exec_domain) NIRVANA_NOEXCEPT;

	void exec_domain (nullptr_t) NIRVANA_NOEXCEPT
	{
		exec_domain_ = nullptr;
		runtime_support_ = nullptr;
	}

	/// Returns special "neutral" execution context with own stack and CPU state.
	ExecContext& neutral_context () NIRVANA_NOEXCEPT
	{
		return neutral_context_;
	}

	/// Returns runtime support object.
	RuntimeSupportImpl& runtime_support () const NIRVANA_NOEXCEPT
	{
		assert (runtime_support_);
		return *runtime_support_;
	}

	virtual void yield () NIRVANA_NOEXCEPT = 0;

protected:
	Thread () :
		exec_domain_ (nullptr),
		neutral_context_ (true)
	{}

protected:
	RuntimeSupportImpl* runtime_support_;
	/// Pointer to the current execution domain.
	ExecDomain* exec_domain_;

private:
	/// Special "neutral" execution context with own stack and CPU state.
	ExecContext neutral_context_;
};

}
}

#endif
