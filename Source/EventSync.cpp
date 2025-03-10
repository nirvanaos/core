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
		
		ExecDomain* next = wait_list_;
		ExecDomain** prev = &wait_list_;

		while (next && next->deadline () < ed.deadline ()) {
			prev = reinterpret_cast <ExecDomain**> (&static_cast <StackElem&> (*next).next);
			next = *prev;
		}

		ed.suspend_prepare ();

		static_cast <StackElem&> (ed).next = next;
		*prev = &ed;

		ed.suspend ();
	}
}

void EventSync::signal () noexcept
{
	assert (!signalled_);
	signalled_ = true;
	while (ExecDomain* ed = wait_list_) {
		wait_list_ = reinterpret_cast <ExecDomain*> (static_cast <StackElem&> (*ed).next);
		ed->resume ();
	}
}

void EventSync::signal (const CORBA::Exception& ex) noexcept
{
	assert (!signalled_);
	signalled_ = true;
	while (ExecDomain* ed = wait_list_) {
		wait_list_ = reinterpret_cast <ExecDomain*> (static_cast <StackElem&> (*ed).next);
		ed->resume (ex);
	}
}

}
}
