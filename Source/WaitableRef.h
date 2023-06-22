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

/// Waitable reference.
/// 
/// \tparam PtrType  Object pointer type.
///                  This may be plain pointer ot smart pointer.
///                  But it can't be a moveable pointer like unique_ptr.
template <typename PtrType>
class WaitableRef : public WaitableRefBase
{
	static_assert (sizeof (PtrType) == sizeof (uintptr_t), "Invalid pointer type");

	friend class WaitList <PtrType>;

public:
	WaitableRef () noexcept
	{}

	/// Construct waitable reference, 
	/// create wait list and assign current execution domain as creator.
	/// 
	/// \param deadline Maximal deadline of the object creation.
	///                 If deadline of the current executuion domain greater,
	///                 it will be temporary decreased.
	/// 
	/// \throw CORBA::BAD_OPERATION if called out of a synchronization domain.
	WaitableRef (TimeBase::TimeT deadline) :
		WaitableRefBase (deadline)
	{}

	/// Construct immediate ready reference without a wait list.
	/// 
	/// \param p Pointer to the created object.
	/// 
	WaitableRef (const PtrType& p) :
		WaitableRefBase (WaitList <PtrType>::make_ptr (p))
	{}

	/// Construct immediate ready reference without a wait list.
	/// 
	/// \param p Pointer to the created object.
	/// 
	WaitableRef (PtrType&& p) noexcept :
		WaitableRefBase (WaitList <PtrType>::make_ptr (std::move (p)))
	{}

	WaitableRef (WaitableRef&&) = default;

	/// Destructor calls ~PtrType if object construction was completed.
	~WaitableRef ()
	{
		reset ();
	}

	WaitableRef& operator = (const PtrType& p) noexcept
	{
		if (is_wait_list ())
			wait_list ()->finish_construction (p);
		else
			WaitList <PtrType>::make_ref (pointer_) = p;

		return *this;
	}

	WaitableRef& operator = (PtrType&& p) noexcept
	{
		if (is_wait_list ())
			wait_list ()->finish_construction (std::move (p));
		else
			WaitList <PtrType>::make_ref (pointer_) = std::move (p);

		return *this;
	}

	/// Get object pointer.
	/// 
	/// \returns Pointer.
	PtrType get () const
	{
		if (is_wait_list ())
			return wait_list ()->wait ();
		else
			return WaitList <PtrType>::make_ref (pointer_);
	}

	/// \brief Get object pointer if object is already constructed.
	/// 
	/// Non-blocking call.
	/// 
	/// \returns Pointer or `nullptr`.
	PtrType get_if_constructed () const noexcept
	{
		if (is_wait_list ())
			return nullptr;
		else
			return WaitList <PtrType>::make_ref (pointer_);
	}

	void reset () noexcept
	{
		if (this->is_wait_list ())
			WaitableRefBase::reset ();
		else {
			uintptr_t up = pointer_;
			if (up) {
				pointer_ = 0;
				reinterpret_cast <PtrType&> (up).~PtrType ();
			}
		}
	}

	WaitListRef <WaitList <PtrType> > wait_list () const noexcept
	{
		return WaitListRef <WaitList <PtrType> >::cast (static_cast <WaitListBase*> (WaitableRefBase::wait_list ()));
	}

	bool is_constructed () const noexcept
	{
		return !is_wait_list ();
	}

};

}
}

#endif
