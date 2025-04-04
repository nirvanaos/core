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
#ifndef NIRVANA_CORE_THREADWORKER_H_
#define NIRVANA_CORE_THREADWORKER_H_
#pragma once

#include "Thread.h"
#include "Executor.h"

namespace Nirvana {
namespace Core {

class Executor;

/// ThreadWorker instances are never created by core.
/// The underlying port implementation creates a pool of ThreadWorker objects on startup.
class ThreadWorker : public Thread
{
public:
	/// This method is called by the port scheduler.
	void execute (Ref <Executor>&& executor) noexcept
	{
		assert (this == current_ptr ());

		// Always called in the neutral context.
		assert (context () == &neutral_context ());

		execute_begin ();

		// Switch to executor context.
		Executor* p = executor;
		p->execute (*this, std::move (executor));

		cleanup ();
	}

};

}
}

#endif
