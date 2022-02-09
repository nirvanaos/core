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
#ifndef NIRVANA_CORE_WAITABLEREF_H_
#define NIRVANA_CORE_WAITABLEREF_H_
#pragma once

#include "WaitList.h"

namespace Nirvana {
namespace Core {

class WaitableRefBase
{
public:
	/// Called by creator execution domain on exception in object creation.
	void on_exception () NIRVANA_NOEXCEPT;

	/// \returns `true` if object creation is not completed.
	bool is_wait_list () const NIRVANA_NOEXCEPT
	{
		return pointer_ & 1;
	}

protected:
	WaitableRefBase (DeadlineTime deadline);
	~WaitableRefBase ();

	WaitableRefBase (WaitableRefBase&& src) NIRVANA_NOEXCEPT :
	pointer_ (src.pointer_)
	{
		src.pointer_ = 0;
	}

	void finish_construction (uintptr_t p) NIRVANA_NOEXCEPT;
	void wait_construction () const;

private:
	WaitList* wait_list () const NIRVANA_NOEXCEPT;
	void detach (CoreRef <WaitList>& ref) NIRVANA_NOEXCEPT;

protected:
	uintptr_t pointer_;
};

/// Waitable reference.
/// 
/// \tparam PtrType  Object pointer type.
///                  This may be plain pointer ot smart pointer.
template <typename PtrType>
class WaitableRef :
	public WaitableRefBase
{
	static_assert (sizeof (PtrType) == sizeof (uintptr_t), "Invalid pointer type");
public:
	/// Construct waitable reference, 
	/// create wait list and assign current execution domain as creator.
	/// 
	/// \param deadline Maximal deadline of the object creation.
	///                 If deadline of the current executuion domain greater,
	///                 it will be temporary decreased.
	/// 
	/// \throw CORBA::BAD_OPERATION if called out of a synchronization domain.
	WaitableRef (DeadlineTime deadline) :
		WaitableRefBase (deadline)
	{}

	WaitableRef (WaitableRef&&) = default;

	/// Destructor calls ~PtrType if object constructin was completed.
	~WaitableRef ()
	{
		if (!this->is_wait_list ())
			this->ref ().~PtrType ();
	}

	/// Called by creator execution domain on finish the object creation.
	/// 
	/// \param p Pointer to the created object.
	void finish_construction (PtrType p) NIRVANA_NOEXCEPT
	{
		uintptr_t up;
		reinterpret_cast <PtrType&> (up) = std::move (p);
		WaitableRefBase::finish_construction (up);
	}

	/// Get object pointer.
	/// 
	/// \returns Pointer.
	const PtrType& get ()
	{
		wait_construction ();
		return reinterpret_cast <const PtrType&> (pointer_);
	}

	/// Get object pointer if object is already constructed.
	/// 
	/// \returns Pointer or `nullptr`.
	PtrType get_if_constructed () const NIRVANA_NOEXCEPT
	{
		if (is_wait_list ())
			return nullptr;
		else
			return reinterpret_cast <const PtrType&> (pointer_);
	}

private:
	PtrType& ref () NIRVANA_NOEXCEPT
	{
		assert (!this->is_wait_list ());
		return reinterpret_cast <PtrType&> (this->pointer_);
	}
};

}
}

#endif
