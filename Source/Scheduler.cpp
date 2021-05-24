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
#include "Scheduler.h"
#include "ExecDomain.h"
#include "initterm.h"

namespace Nirvana {
namespace Core {

void Scheduler::Terminator::run ()
{
	terminate ();
}

Scheduler::GlobalData Scheduler::global_;

void Scheduler::shutdown () NIRVANA_NOEXCEPT
{
	State state = State::RUNNING;
	if (global_.state.compare_exchange_strong (state, State::SHUTDOWN_STARTED) && !global_.activity_cnt) {
		global_.activity_cnt.increment ();
		activity_end ();
	}
}

void Scheduler::activity_end () NIRVANA_NOEXCEPT
{
	if (!global_.activity_cnt.decrement ()) {
		switch (global_.state) {
			case State::SHUTDOWN_STARTED: {
				State state = State::SHUTDOWN_STARTED;
				if (global_.state.compare_exchange_strong (state, State::TERMINATE))
					ExecDomain::async_call (INFINITE_DEADLINE, global_.terminator, g_core_free_sync_context);
			} break;

			case State::TERMINATE: {
				State state = State::TERMINATE;
				if (global_.state.compare_exchange_strong (state, State::SHUTDOWN_FINISH))
					Port::Scheduler::shutdown ();
			} break;
		}
	}
}

}
}
