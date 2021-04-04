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

AtomicCounter <false> Scheduler::activity_cnt_ (0);
std::atomic <Scheduler::State> Scheduler::state_ (State::RUNNING);

void Scheduler::shutdown () NIRVANA_NOEXCEPT
{
	State state = State::RUNNING;
	if (state_.compare_exchange_strong (state, State::SHUTDOWN_STARTED) && !activity_cnt_) {
		activity_cnt_.increment ();
		activity_end ();
	}
}

void Scheduler::activity_begin ()
{
	activity_cnt_.increment ();
	if (State::RUNNING != state_) {
		activity_end ();
		throw CORBA::INITIALIZE ();
	}
}

inline
void Scheduler::check_shutdown () NIRVANA_NOEXCEPT
{
	State state = State::SHUTDOWN_STARTED;
	if (state_.compare_exchange_strong (state, State::SHUTDOWN_FINISH)) {
		terminate ();
		Port::Scheduler::shutdown ();
	}
}

void Scheduler::activity_end () NIRVANA_NOEXCEPT
{
	if (!activity_cnt_.decrement ())
		check_shutdown ();
}

}
}
