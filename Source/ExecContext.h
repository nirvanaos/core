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

#include "Runnable.h"
#include <Port/ExecContext.h>
#include <Port/Thread.h>
#include "CoreInterface.h"

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

	static ExecContext& current () NIRVANA_NOEXCEPT
	{
		ExecContext* p = Port::Thread::context ();
		assert (p);
		return *p;
	}

	template <class ... Args>
	ExecContext (Args ... args) :
		Port::ExecContext (std::forward <Args> (args)...),
		runnable_ (nullptr)
	{}

	/// Switch to this context.
	void switch_to () NIRVANA_NOEXCEPT
	{
		assert (&current () != this);
		Port::ExecContext::switch_to ();
	}

	static void neutral_context_loop () NIRVANA_NOEXCEPT;

	void run_in_context (Runnable& runnable) NIRVANA_NOEXCEPT
	{
		assert (this != &current ());
		runnable_ = &runnable;
		switch_to ();
	}

	/// Aborts execution of the context.
	/// Dangerous method used for POSIX compatibility.
	NIRVANA_NORETURN void abort ()
	{
		Port::ExecContext::abort ();
	}

protected:
	void run () NIRVANA_NOEXCEPT;

	void on_crash (CORBA::SystemException::Code err) NIRVANA_NOEXCEPT;

protected:
	CoreRef <Runnable> runnable_;
};

}
}

#endif
