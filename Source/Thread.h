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
#pragma once

#include <Port/Thread.h>
#include "ExecContext.h"
#include "Security.h"

namespace Nirvana {
namespace Core {

class ExecDomain;

/// Thread class. Base for ThreadWorker and ThreadBackground.
class Thread : protected Port::Thread
{
	friend class Port::Thread;

public:
	// Implementation - specific methods can be called explicitly.
	Port::Thread& port () noexcept
	{
		return *this;
	}

	/// Returns current thread.
	static Thread& current () noexcept
	{
		Thread* p = Port::Thread::current ();
		assert (p);
		return *p;
	}

	/// Returns current thread.
	/// May return nullptr if called from a special port thread.
	static Thread* current_ptr () noexcept
	{
		return Port::Thread::current ();
	}

	/// Returns ExecDomain assigned to thread.
	ExecDomain* exec_domain () const noexcept
	{
		assert (executing_);
		return exec_domain_;
	}

	/// Assign ExecDomain to thread
	void exec_domain (ExecDomain& exec_domain) noexcept
	{
		assert (!exec_domain_ || exec_domain_ == &exec_domain);
		exec_domain_ = &exec_domain;
	}

	/// Returns special "neutral" execution context with own stack and CPU state.
	ExecContext& neutral_context () noexcept
	{
		return neutral_context_;
	}

	/// Release worker thread.
	/// This method is called by Executor from the neutral context when execution is completed.
	void execute_end () noexcept
	{
		assert (executing_);
		executing_ = false;
	}

	bool executing () const noexcept
	{
		return executing_;
	}

	static void impersonate (const Security::Context& sec_context)
	{
		Port::Thread::impersonate (sec_context.port ());
	}

protected:
	Thread ();
	~Thread ();

	void execute_begin () noexcept
	{
		assert (!executing_);
		executing_ = true;
	}

protected:
	// Pointer to the current execution domain.
	ExecDomain* exec_domain_;

private:
	// Special "neutral" execution context with own stack and CPU state.
	ExecContext neutral_context_;

	bool executing_;
};

}
}

#endif
