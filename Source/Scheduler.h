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
#ifndef NIRVANA_CORE_SCHEDULER_H_
#define NIRVANA_CORE_SCHEDULER_H_

#include <Port/Scheduler.h>
#include "AtomicCounter.h"
#include "Runnable.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class Scheduler : public Port::Scheduler
{
public:
	static void shutdown () NIRVANA_NOEXCEPT;

	static void activity_begin ()
	{
		assert (global_.state < State::SHUTDOWN_FINISH);
		global_.activity_cnt.increment ();
	}

	static void activity_end () NIRVANA_NOEXCEPT;

private:
	enum State
	{
		RUNNING = 0,
		SHUTDOWN_STARTED,
		TERMINATE,
		SHUTDOWN_FINISH
	};

	class Terminator : public ImplStatic <Runnable>
	{
	private:
		virtual void run ();
	};

	struct GlobalData
	{
		std::atomic <State> state;
		AtomicCounter <false> activity_cnt;
		Terminator terminator;

		GlobalData () :
			state (State::RUNNING),
			activity_cnt (0)
		{}
	};

	static GlobalData global_;
};

}
}

#endif
