/// \file
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
#include "ThreadBackground.h"
#include "../ExecDomain.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

using namespace Nirvana::Core;

ThreadBackground::ThreadBackground (bool process) :
	Nirvana::Core::Port::ThreadBackground (process)
{}

void ThreadBackground::start (Nirvana::Core::Runnable& runnable, Nirvana::Core::MemContext& mem_context)
{
	auto ed = ExecDomain::create_background (*this, runnable, mem_context);
	exec_domain_ = ed;
	ed->start ([this]() {this->create (); });
	_add_ref ();
}

void ThreadBackground::schedule_call (Nirvana::Core::SyncContext& target, MemContext* mem_context)
{
	// We don't switch context if no sync domain
	if (target.sync_domain ()) {
		ExecDomain* exec_domain = Thread::current ().exec_domain ();
		assert (exec_domain);
		exec_domain->schedule_call (target, mem_context);
		check_schedule_error (*exec_domain);
	}
}

void ThreadBackground::schedule_return (Nirvana::Core::ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
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
