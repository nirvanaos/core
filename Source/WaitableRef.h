/// \file
/// Currently unused. TODO: Remove.
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
#ifndef NIRVANA_CORE_WAITABLEREF_H_
#define NIRVANA_CORE_WAITABLEREF_H_
#pragma once

#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

class WaitListImpl;
typedef ImplDynamicSync <WaitListImpl> WaitList;

template <class PtrType, uint64_t deadline> class WaitableRef;

class WaitableRefBase
{
public:
	void on_exception () NIRVANA_NOEXCEPT;

	bool is_wait_list () const NIRVANA_NOEXCEPT
	{
		return pointer_ & 1;
	}

protected:
	WaitableRefBase (uint64_t deadline);
	~WaitableRefBase ();

	void initialize (uintptr_t p) NIRVANA_NOEXCEPT;
	void wait_construction () const;

private:
	WaitList* wait_list () const NIRVANA_NOEXCEPT;
	void detach (CoreRef <WaitList>& ref) NIRVANA_NOEXCEPT;

protected:
	uintptr_t pointer_;
};

template <class PtrType, uint64_t deadline>
class WaitableRefBaseT :
	public WaitableRefBase
{
public:
	const PtrType& initialize (PtrType p) NIRVANA_NOEXCEPT
	{
		uintptr_t up;
		reinterpret_cast <PtrType&> (up) = std::move (p);
		WaitableRefBase::initialize (up);
		return reinterpret_cast <PtrType&> (pointer_);
	}

	const PtrType& get ()
	{
		wait_construction ();
		return reinterpret_cast <const PtrType&> (pointer_);
	}

	PtrType get_constructed () const
	{
		if (is_wait_list ())
			return nullptr;
		else
			return reinterpret_cast <const PtrType&> (pointer_);
	}

protected:
	WaitableRefBaseT () :
		WaitableRefBase (deadline)
	{}

	PtrType& ref () NIRVANA_NOEXCEPT
	{
		assert (!this->is_wait_list ());
		return reinterpret_cast <PtrType&> (this->pointer_);
	}

private:
	static_assert (sizeof (PtrType) == sizeof (uintptr_t), "Invalid pointer type");
};

template <class T, uint64_t deadline>
class WaitableRef <T*, deadline> :
	public WaitableRefBaseT <T*, deadline>
{
public:
	~WaitableRef ()
	{
		if (!this->is_wait_list ())
			delete this->ref ();
	}
};

template <class T, uint64_t deadline>
class WaitableRef <CoreRef <T>, deadline> :
	public WaitableRefBaseT <CoreRef <T>, deadline>
{
public:
	~WaitableRef ()
	{
		if (!this->is_wait_list ())
			this->ref ().~CoreRef ();
	}
};

}
}

#endif
