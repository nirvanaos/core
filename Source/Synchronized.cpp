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
#include "Synchronized.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

Synchronized::Synchronized (SyncContext& target, MemContext* mem_context) :
	call_context_ (&SyncContext::current ()),
	exec_domain_ (Thread::current ().exec_domain ())
{
	assert (exec_domain_);
	// Target can not be legacy thread
	assert (target.sync_domain () || target.is_free_sync_context ());
	exec_domain_->schedule_call (target, mem_context);
}

Synchronized::~Synchronized ()
{
	if (call_context_) // If no exception and no suspend_and_return ()
		exec_domain_->schedule_return (*call_context_);
}

NIRVANA_NORETURN void Synchronized::on_exception ()
{
	std::exception_ptr ex = std::current_exception ();
	CoreRef <SyncContext> context = std::move (call_context_);
	exec_domain_->schedule_return (*context);
	std::rethrow_exception (ex);
}

void Synchronized::suspend_and_return ()
{
	CoreRef <SyncContext> context = std::move (call_context_);
	exec_domain_->suspend (context);
}

}
}
