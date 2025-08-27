/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#ifndef NIRVANA_CORE_INITONCE_H_
#define NIRVANA_CORE_INITONCE_H_
#pragma once

#include "Event.h"
#include "LockablePtr.h"
#include "AtomicCounter.h"
#include <Nirvana/native.h>

namespace Nirvana {
namespace Core {

class InitOnce :
	private UserObject,
	private Event
{
public:
	// control must be placed in a separate read-write section
	static void once (Pointer& control, InitFunc init_func)
	{
		typedef LockablePtrT <InitOnce, 1> Ptr;

		Ptr& lockable = reinterpret_cast <Ptr&> (control);
		if (lockable.load () == Ptr::Ptr (nullptr, 1))
			return;

		InitOnce* ev = nullptr;

		Ptr::Ptr p = lockable.lock ();
		if (static_cast <InitOnce*> (p)) {
			ev = p;
			ev->_add_ref ();
			lockable.unlock ();
		} else {
			lockable.unlock ();
			Ptr::Ptr pev (new InitOnce);
			if (lockable.cas (Ptr::Ptr (), pev)) {
				ev = pev;
				(*init_func) ();
				ev->signal ();
				lockable.exchange (Ptr::Ptr (nullptr, 1));
				ev->_remove_ref ();
				return;
			} else {
				p = lockable.lock ();
				if (static_cast <InitOnce*> (p)) {
					ev = p;
					ev->_add_ref ();
				}
				lockable.unlock ();
			}
		}

		if (ev) {
			ev->wait ();
			ev->_remove_ref ();
		}
	}

private:
	InitOnce ()
	{}

	~InitOnce ()
	{}

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept
	{
		if (0 == ref_cnt_.decrement ())
			delete this;
	}

private:
	RefCounter ref_cnt_;
};

}
}

#endif
