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
#include "WaitList.h"
#include "CoreObject.h"
#include "Suspend.h"

using namespace std;

namespace Nirvana {
namespace Core {

WaitListImpl::WaitListImpl () :
#ifdef _DEBUG
	finished_ (false),
#endif
	wait_list_ (nullptr)
{
	if (!SyncContext::current ().sync_domain ())
		throw_BAD_INV_ORDER ();
}

void WaitListImpl::wait ()
{
	assert (!finished_);
	// Hold reference to this object
	CoreRef <WaitList> ref = static_cast <WaitList*> (this);
	ExecDomain::Impl* ed = static_cast <ExecDomain::Impl*> (Thread::current ().exec_domain ());
	assert (ed);
	static_cast <StackElem&> (*ed).next = wait_list_;
	wait_list_ = ed;
	ed->suspend ();
	assert (finished_);
	if (exception_)
		rethrow_exception (exception_);
}

void WaitListImpl::on_exception () NIRVANA_NOEXCEPT
{
	assert (!finished_);
	assert (!exception_);
	exception_ = current_exception ();
	finish ();
}

void WaitListImpl::finish () NIRVANA_NOEXCEPT
{
#ifdef _DEBUG
	finished_ = true;
#endif
	while (ExecDomain::Impl* ed = wait_list_) {
		wait_list_ = reinterpret_cast <ExecDomain::Impl*> (static_cast <StackElem&> (*ed).next);
		ed->resume ();
	}
}

}
}
