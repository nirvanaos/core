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
#include "Mutex.h"
#include "../ExecDomain.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

MutexCore::~MutexCore ()
{
	// TODO: Terminate all waiting threads
}

void MutexCore::lock ()
{
	ThreadLegacy& thread = ThreadLegacy::current ();
	SYNC_BEGIN (*this, nullptr);
	if (!owner_) {
		owner_ = &thread;
		return;
	} else if (owner_ == &thread)
		throw_BAD_INV_ORDER ();
	else {
		queue_.push_back (thread);
		_sync_frame.suspend_and_return ();
	}
	SYNC_END ();
}

void MutexCore::unlock ()
{
	ThreadLegacy& thread = ThreadLegacy::current ();
	SYNC_BEGIN (*this, nullptr);
	if (owner_ != &thread)
		throw_BAD_INV_ORDER ();
	owner_ = nullptr;
	if (!queue_.empty ()) {
		ThreadLegacy& next = queue_.front ();
		next.remove (); // From queue
		owner_ = &next;
		next.exec_domain ()->resume ();
	}
	SYNC_END ();
}

}
}
}