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
#include "ScheduleReturn.h"
#include "Thread.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

void ScheduleReturn::run ()
{
	Thread& thread = Thread::current ();
	ExecDomain* exec_domain = thread.exec_domain ();
	assert (exec_domain);
	Core_var <SyncDomain> old_sync_domain = exec_domain->sync_context ()->sync_domain ();
	sync_context_.schedule_return (*exec_domain);
	if (old_sync_domain)
		old_sync_domain->leave ();
	thread.yield ();
}

}
}
