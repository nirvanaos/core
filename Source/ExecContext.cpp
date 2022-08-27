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
#include "ExecDomain.h"
#include "Thread.h"

namespace Nirvana {
namespace Core {

void ExecContext::run () NIRVANA_NOEXCEPT
{
	assert (runnable_);
	try {
		runnable_->run ();
	} catch (...) {
		runnable_->on_exception ();
	}
	runnable_.reset ();
}

void ExecContext::on_crash (const siginfo& signal) NIRVANA_NOEXCEPT
{
	runnable_->on_crash (signal);
	runnable_.reset ();
}

void ExecContext::neutral_context_loop () NIRVANA_NOEXCEPT
{
	Thread& thread = Thread::current ();
	assert (&current () == &thread.neutral_context ());
	for (;;) {
		thread.neutral_context ().run ();
		if (thread.exec_domain ())
			thread.exec_domain ()->switch_to ();
		else
			break;
	}
}

}
}
