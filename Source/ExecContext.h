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
#ifndef NIRVANA_CORE_EXECCONTEXT_H_
#define NIRVANA_CORE_EXECCONTEXT_H_
#pragma once

#include "Runnable.h"
#include <assert.h>
#include <Port/ExecContext.h>
#include <Port/Thread.h>

namespace Nirvana {
namespace Core {

/// Execution context (coroutine, fiber).
class ExecContext :
	protected Port::ExecContext
{
	friend class Port::ExecContext;
public:
	// Implementation - specific methods must be called explicitly.
	Port::ExecContext& port () NIRVANA_NOEXCEPT
	{
		return *this;
	}

	/// \returns Current execution context.
	static ExecContext& current () NIRVANA_NOEXCEPT
	{
		ExecContext* p = current_ptr ();
		assert (p);
		return *p;
	}

	/// \returns Current execution context pointer.
	///          Returns `nullptr` for special non-worker threads.
	static ExecContext* current_ptr () NIRVANA_NOEXCEPT
	{
		return Port::Thread::context ();
	}

	/// Constructor.
	ExecContext () :
		ExecContext (false)
	{}

	/// Switch to this context.
	void switch_to () NIRVANA_NOEXCEPT
	{
		assert (&current () != this);
		Port::ExecContext::switch_to ();
	}

	static void neutral_context_loop () NIRVANA_NOEXCEPT;

	/// Raise signal.
	NIRVANA_NORETURN void raise (int signal)
	{
		Port::ExecContext::raise (signal);
	}

protected:
	ExecContext (bool neutral) :
		Port::ExecContext (neutral),
		runnable_ (nullptr)
	{}

	void run ();
	void on_crash (const siginfo& signal) NIRVANA_NOEXCEPT;

protected:
	Runnable* runnable_;
};

class NeutralContext : public ExecContext
{
public:
	NeutralContext () :
		ExecContext (true)
	{}

	void run_in_context (Runnable& runnable)
	{
		assert (this != &current ());
		runnable_ = &runnable;
		switch_to ();
		if (exception_) {
			std::exception_ptr tmp (std::move (exception_));
			std::rethrow_exception (tmp);
		}
	}

	void run () NIRVANA_NOEXCEPT
	{
		try {
			ExecContext::run ();
		} catch (...) {
			exception_ = std::current_exception ();
		}
	}

private:
	std::exception_ptr exception_;
};

}
}

#endif
