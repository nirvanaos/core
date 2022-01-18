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
#include "Process.h"
#include "../ExecDomain.h"

using namespace Nirvana::Core;

namespace Nirvana {
namespace Legacy {
namespace Core {

void ThreadBackground::start (Process& process, Nirvana::Core::Runnable& runnable)
{
	auto ed = ExecDomain::create_background (*this, runnable, process);
	exec_domain_ = ed;
	ed->start ([this]() {this->create (); });
	_add_ref ();
}

void ThreadBackground::schedule_call (SyncContext& target, MemContext* mem_context)
{
	assert (this == Thread::current_ptr ());

	// We don't switch context if no sync domain
	ExecDomain* ed = exec_domain ();
	if (target.sync_domain ()) {
		ed->schedule_call (target, mem_context);
		check_schedule_error (*ed);
	} else
		ed->mem_context_push (mem_context);
}

void ThreadBackground::schedule_return (Nirvana::Core::ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
	assert (&exec_domain == this->exec_domain ());
	exec_domain.sync_context (*this);
	Port::ThreadBackground::resume ();
}

void ThreadBackground::yield () NIRVANA_NOEXCEPT
{
	Port::ThreadBackground::suspend ();
}

}
}
}
