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

namespace Nirvana {
namespace Core {

ImplStatic <SyncContextCore> g_core_free_sync_context;

SyncContext& SyncContext::current () NIRVANA_NOEXCEPT
{
	ExecDomain* ed = Thread::current ().exec_domain ();
	assert (ed);
	return ed->sync_context ();
}

void SyncContext::check_schedule_error (ExecDomain& ed)
{
	CORBA::Exception::Code err = Thread::current ().exec_domain ()->scheduler_error ();
	if (err >= 0) {
		// We must return to prev synchronization domain back before throwing the exception.
		ed.schedule_return (*this);
		CORBA::SystemException::_raise_by_code (err);
	}
}

SyncDomain* SyncContext::sync_domain () NIRVANA_NOEXCEPT
{
	return nullptr;
}

Heap* SyncContext::stateless_memory () NIRVANA_NOEXCEPT
{
	return nullptr;
}

void SyncContextFree::schedule_call (SyncContext& target)
{
	ExecDomain* exec_domain = Thread::current ().exec_domain ();
	assert (exec_domain);
	exec_domain->schedule_call (target);
	check_schedule_error (*exec_domain);
}

void SyncContextFree::schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
	exec_domain.sync_context (*this);
	Scheduler::schedule (exec_domain.deadline (), exec_domain);
}

Heap& SyncContextFree::memory () NIRVANA_NOEXCEPT
{
	return Thread::current ().exec_domain ()->heap ();
}

RuntimeSupport& SyncContextFree::runtime_support () NIRVANA_NOEXCEPT
{
	return Thread::current ().exec_domain ()->runtime_support ();
}

Heap* SyncContextCore::stateless_memory () NIRVANA_NOEXCEPT
{
	return &g_core_heap;
}

}
}
