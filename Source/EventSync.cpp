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
#include "EventSync.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

EventSync::EventSync (bool signalled) :
	wait_list_ (nullptr),
	signalled_ (signalled)
{
	if (!SyncContext::current ().sync_domain ())
		throw_BAD_OPERATION ();
}

void EventSync::wait ()
{
	if (!signalled_) {
		ExecDomain& ed = ExecDomain::current ();
		static_cast <StackElem&> (ed).next = wait_list_;
		wait_list_ = &ed;
		try {
			ed.suspend ();
		} catch (...) {
			wait_list_ = reinterpret_cast <ExecDomain*> (static_cast <StackElem&> (ed).next);
			throw;
		}
	}
	assert (signalled_);
}

void EventSync::signal () noexcept
{
	// TODO: Can cause priority inversion.
	// We should sort released ED by deadline.
	assert (!signalled_);
	signalled_ = true;
	while (ExecDomain* ed = wait_list_) {
		wait_list_ = reinterpret_cast <ExecDomain*> (static_cast <StackElem&> (*ed).next);
		ed->resume ();
	}
}

}
}
