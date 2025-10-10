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
#include "SyncContext.h"
#include <new>

namespace Nirvana {
namespace Core {

WaitListRef <WaitListBase> WaitableRefBase::detach_list () noexcept
{
	assert (is_wait_list ());
	uintptr_t up = pointer_ & ~1;
	pointer_ = 0;
	return reinterpret_cast <WaitListRef <WaitListBase>&&> (up);
}

bool WaitableRefBase::initialize (TimeBase::TimeT deadline)
{
	assert (SyncContext::current ().sync_domain ());
	if (!pointer_) {
		::new (&pointer_) WaitListRef <WaitListBase> (WaitListRef <WaitListBase>::create <WaitListImpl <WaitListBase> > (deadline, this));
		assert (!(pointer_ & 1));
		pointer_ |= 1;
		return true;
	}
	return false;
}

void WaitableRefBase::reset () noexcept
{
	assert (is_wait_list ());
	auto wait_list = detach_list ();
	wait_list->on_reset_ref ();
}

WaitListRef <WaitListBase> WaitableRefBase::finish_construction (void* p) noexcept
{
	assert (!((uintptr_t)p & 1));
	WaitListRef <WaitListBase> wait_list = detach_list ();
	pointer_ = (uintptr_t)p;
	return wait_list;
}

const uintptr_t& WaitableRefBase::wait_construction () const
{
	if (is_wait_list ())
		return wait_list ()->wait ();
	else
		return pointer_;
}

}
}
