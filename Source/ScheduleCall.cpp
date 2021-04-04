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
#include "ScheduleCall.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

void ScheduleCall::schedule_call (SyncDomain* sync_domain)
{
	ScheduleCall runnable (sync_domain);
	run_in_neutral_context (runnable);
	if (runnable.exception_)
		std::rethrow_exception (runnable.exception_);
}

void ScheduleCall::run ()
{
	ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	exec_domain->schedule (sync_domain_);
	Thread::current ().yield ();
}

void ScheduleCall::on_exception () NIRVANA_NOEXCEPT
{
	exception_ = std::current_exception ();
}

}
}
