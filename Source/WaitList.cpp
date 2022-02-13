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
#include "Chrono.h"

using namespace std;

namespace Nirvana {
namespace Core {

WaitListImpl::WaitListImpl (DeadlineTime deadline) :
	worker_ (&ExecDomain::current ()),
	worker_deadline_ (worker_->deadline ()),
	wait_list_ (nullptr)
{
	if (!SyncContext::current ().sync_domain ())
		throw_BAD_OPERATION ();
	DeadlineTime max_dt = Chrono::make_deadline (deadline);
	if (worker_deadline_ > max_dt)
		worker_->deadline (max_dt);
}

void WaitListImpl::wait ()
{
	if (!finished ()) {
		ExecDomain& ed = ExecDomain::current ();
		if (&ed == worker_)
			throw_BAD_INV_ORDER ();
		// Hold reference to this object
		CoreRef <WaitList> ref = static_cast <WaitList*> (this);
		static_cast <StackElem&> (ed).next = wait_list_;
		wait_list_ = &ed;
		ed.suspend ();
	}
	assert (finished ());
	if (exception_)
		rethrow_exception (exception_);
}

void WaitListImpl::on_exception () NIRVANA_NOEXCEPT
{
	assert (!finished ());
	assert (!exception_);
	exception_ = current_exception ();
	finish ();
}

void WaitListImpl::finish () NIRVANA_NOEXCEPT
{
	assert (!finished ());
	assert (&ExecDomain::current () == worker_);
	worker_->deadline (worker_deadline_);
	worker_ = nullptr;
	while (ExecDomain* ed = wait_list_) {
		wait_list_ = reinterpret_cast <ExecDomain*> (static_cast <StackElem&> (*ed).next);
		ed->resume ();
	}
}

}
}
