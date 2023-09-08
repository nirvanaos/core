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
	wait_op_ (std::ref (*this)),
	signalled_ (signalled)
{
}

void Event::wait () {
	if (!signalled_) {
		ExecDomain::current ().suspend_prepare ();
		ExecContext::run_in_neutral_context (wait_op_);
	}
}

void Event::WaitOp::run ()
{
	ExecDomain& ed = ExecDomain::current ();
	ed.suspend_prepared ();
	obj_.push (ed);
	if (obj_.signalled_) {
		ExecDomain* ed = obj_.pop ();
		if (ed)
			ed->resume ();
	}
}

void Event::signal () noexcept
{
	// TODO: Can cause priority inversion.
	// We should sort released ED by deadline.
	assert (!signalled_);
	signalled_ = true;
	while (ExecDomain* ed = pop ()) {
		ed->resume ();
	}
}

}
}
