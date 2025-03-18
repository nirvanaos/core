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
#include "pch.h"
#include "Event.h"

namespace Nirvana {
namespace Core {

Event::Event (bool signalled) noexcept :
	signalled_ (signalled)
{
}

void Event::wait ()
{
	if (!signalled_) {
		Thread& thread = Thread::current ();
		ExecDomain& cur_ed = *thread.exec_domain ();
		bool pop_qnode = cur_ed.suspend_prepare_no_leave ();
		push (cur_ed);
		if (signalled_) {
			cur_ed.suspend_unprepare (pop_qnode);
			resume_all ();
			eh_.check ();
		} else {
			Port::Thread::PriorityBoost boost (&thread);
			cur_ed.leave_and_suspend ();
		}
	} else
		resume_all (); // Help others
}

void Event::resume_all () noexcept
{
	Port::Thread::PriorityBoost boost;
	while (ExecDomain* ed = pop ()) {
		if (ed->suspended ())
			ed->resume (eh_);
	}
}

void Event::signal () noexcept
{
	assert (!signalled_);
	signalled_ = true;
	resume_all ();
}

void Event::signal (const CORBA::Exception& ex) noexcept
{
	assert (!signalled_);
	eh_.set_exception (ex);
	signalled_ = true;
	resume_all ();
}

void Event::reset () noexcept
{
	assert (signalled_);
	if (signalled_) {
		signalled_ = false;
		resume_all ();
	}
	eh_.reset ();
}

}
}
