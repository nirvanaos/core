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
#include "SyncContext.h"
#include "Thread.h"
#include "ExecDomain.h"
#include "ScheduleCall.h"
#include "ScheduleReturn.h"
#include "Suspend.h"

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE FreeSyncContext :
	public SyncContext
{
public:
	virtual void schedule_call (SyncDomain* sync_domain);
	virtual void schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT;
	virtual SyncDomain* sync_domain () NIRVANA_NOEXCEPT;
	virtual Heap& memory () NIRVANA_NOEXCEPT;
};

ImplStatic <FreeSyncContext> g_free_sync_context;

SyncContext& SyncContext::current () NIRVANA_NOEXCEPT
{
	ExecDomain* ed = Thread::current ().exec_domain ();
	assert (ed);
	SyncContext* sc = ed->sync_context ();
	assert (sc);
	return *sc;
}

SyncContext& SyncContext::free_sync_context () NIRVANA_NOEXCEPT
{
	return g_free_sync_context;
}

void SyncContext::check_schedule_error ()
{
	CORBA::Exception::Code err = Thread::current ().exec_domain ()->scheduler_error ();
	if (err) {
		// We must return to prev synchronization domain back before throwing the exception.
		ScheduleReturn::schedule_return (*this);
		CORBA::SystemException::_raise_by_code (err);
	}
}

void FreeSyncContext::schedule_call (SyncDomain* sync_domain)
{
	if (SyncContext::SUSPEND () == sync_domain)
		Suspend::suspend ();
	else {
		ScheduleCall::schedule_call (sync_domain);
		check_schedule_error ();
	}
}

void FreeSyncContext::schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
	exec_domain.sync_context (*this);
	Scheduler::schedule (exec_domain.deadline (), exec_domain);
}

SyncDomain* FreeSyncContext::sync_domain () NIRVANA_NOEXCEPT
{
	return nullptr;
}

Heap& FreeSyncContext::memory () NIRVANA_NOEXCEPT
{
	Thread& th = Thread::current ();
	return th.exec_domain ()->heap ();
}

}
}
