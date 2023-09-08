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
#include "pch.h"
#include "WaitableRef.h"
#include "ExecDomain.h"
#include "Chrono.h"

namespace Nirvana {
namespace Core {

WaitListBase::WaitListBase (TimeBase::TimeT deadline, WaitableRefBase* ref) :
	worker_ (&ExecDomain::current ()),
	result_ (0),
	ref_ (ref),
	worker_deadline_ (worker_->deadline ())
{
	DeadlineTime max_dt = Chrono::make_deadline (deadline);
	if (worker_deadline_ > max_dt)
		worker_->deadline (max_dt);
}

uintptr_t& WaitListBase::wait ()
{
	if (!finished ()) {
		ExecDomain& ed = ExecDomain::current ();
		if (&ed == worker_)
			throw_BAD_INV_ORDER ();
		// Hold reference to this object
		WaitListRef <WaitListBase> hold (static_cast <WaitListImpl <WaitListBase>*> (this));
		event_.wait ();
	}
	assert (finished ());
	if (exception_)
		std::rethrow_exception (exception_);
	return result_;
}

void WaitListBase::on_exception () noexcept
{
	assert (!finished ());
	assert (!exception_);
	exception_ = std::current_exception ();
	finish (nullptr);
}

void WaitListBase::finish (void* result) noexcept
{
	assert (!finished ());
	assert (&ExecDomain::current () == worker_);
	worker_->deadline (worker_deadline_);
	worker_ = nullptr;
	result_ = (uintptr_t)result;
	event_.signal ();
}

}
}
