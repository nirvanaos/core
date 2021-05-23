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
#include "../Suspend.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

using namespace Nirvana::Core;

ThreadBackground::ThreadBackground (bool process) :
	Nirvana::Core::Port::ThreadBackground (process)
{}

void ThreadBackground::start (RuntimeSupportLegacy& runtime_support, Nirvana::Core::Runnable& runnable)
{
	auto ed = ExecDomain::get_background (*this, runnable);
	exec_domain_ = ed;
	ed->start ([this]() {this->create (); });
	_add_ref ();
}

::Nirvana::Core::SyncDomain* ThreadBackground::sync_domain () NIRVANA_NOEXCEPT
{
	return nullptr;
}

void ThreadBackground::schedule_call (Nirvana::Core::SyncDomain* sync_domain)
{
	// We don't switch context if sync_domain == nullptr
	if (sync_domain) {
		if (SyncContext::SUSPEND () == sync_domain)
			Suspend::suspend ();
		else {
			ExecDomain* exec_domain = Thread::current ().exec_domain ();
			assert (exec_domain);
			exec_domain->schedule_call (sync_domain);
			check_schedule_error (*exec_domain);
		}
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
