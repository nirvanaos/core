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
#include "ThreadWorker.h"
#include "Scheduler.h"
#include "Thread.h"

namespace Nirvana {
namespace Core {

/// This static method is called by the scheduler.
void ThreadWorker::execute (Executor& executor, Word scheduler_error) NIRVANA_NOEXCEPT
{
	// Always called in the neutral context.
	assert (&ExecContext::current () == &Core::Thread::current ().neutral_context ());
	// Switch to executor context.
	executor.execute (scheduler_error);
	// Perform possible neutral context calls, then return.
	ExecContext::neutral_context_loop ();
}

void ThreadWorker::yield () NIRVANA_NOEXCEPT
{
	exec_domain (nullptr); // Release worker thread to a pool.
}

}
}
