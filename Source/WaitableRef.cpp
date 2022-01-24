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
#include "WaitableRef.h"
#include "WaitList.h"
#include <new>

namespace Nirvana {
namespace Core {

inline
WaitList* WaitableRefBase::wait_list () const NIRVANA_NOEXCEPT
{
	assert (is_wait_list ());
	return reinterpret_cast <WaitList*> (pointer_ & ~1);
}

void WaitableRefBase::detach (CoreRef <WaitList>& ref) NIRVANA_NOEXCEPT
{
	assert (is_wait_list ());
	uintptr_t up = pointer_ & ~1;
	pointer_ = 0;
	ref = std::move (reinterpret_cast <CoreRef <WaitList>&> (up));
	assert (up == 0);
}

WaitableRefBase::WaitableRefBase (DeadlineTime deadline)
{
	::new (&pointer_) CoreRef <WaitList> (CoreRef <WaitList>::create <WaitList> (deadline));
	assert (!(pointer_ & 1));
	pointer_ |= 1;
}

WaitableRefBase::~WaitableRefBase ()
{
	if (is_wait_list ()) {
		CoreRef <WaitList> wait_list;
		detach (wait_list);
	}
}

void WaitableRefBase::on_exception () NIRVANA_NOEXCEPT
{
	assert (is_wait_list ());
	wait_list ()->on_exception ();
}

void WaitableRefBase::finish_construction (uintptr_t p) NIRVANA_NOEXCEPT
{
	assert (!(p & 1));
	CoreRef <WaitList> wait_list;
	detach (wait_list);
	pointer_ = p;
	wait_list->finish ();
}

void WaitableRefBase::wait_construction () const
{
	if (is_wait_list ())
		wait_list ()->wait ();
}

}
}
