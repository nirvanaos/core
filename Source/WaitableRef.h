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
	WaitableRefBase (const WaitableRefBase&) = delete;
	WaitableRefBase& operator = (const WaitableRefBase&) = delete;
public:
	/// Called by creator execution domain on exception in object creation.
	void on_exception () noexcept;

	/// \returns `true` if object creation is not completed.
	bool is_wait_list () const noexcept
	{
		return is_wait_list (pointer_);
	}

	bool initialized () const noexcept
	{
		return pointer_ != 0;
	}

	bool initialize (TimeBase::TimeT deadline);

protected:
	WaitableRefBase () noexcept :
		pointer_ (0)
	{}

	WaitableRefBase (TimeBase::TimeT deadline) :
		pointer_ (0)
	{
		initialize (deadline);
	}
	
	WaitableRefBase (WaitableRefBase&& src) noexcept :
		pointer_ (src.pointer_)
	{
		src.pointer_ = 0;
	}

	WaitableRefBase (void* p) noexcept :
		pointer_ ((uintptr_t)p)
	{
		assert (!is_wait_list ());
	}

	~WaitableRefBase ()
	{
		reset ();
	}

	void finish_construction (void* p) noexcept;
	void wait_construction () const;

	void reset () noexcept;

private:
	static bool is_wait_list (uintptr_t pointer) noexcept
	{
		return pointer & 1;
	}

	inline WaitList* wait_list () const noexcept;
	void detach (Ref <WaitList>& ref) noexcept;

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
	WaitableRef ()
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

	/// Construct waitable reference without a wait list.
	/// 
	/// \param p Pointer to the created object.
	/// 
	WaitableRef (const PtrType& p) :
		WaitableRefBase (make_ptr (p))
	{}

	/// Construct waitable reference without a wait list.
	/// 
	/// \param p Pointer to the created object.
	/// 
	WaitableRef (PtrType&& p) :
		WaitableRefBase (make_ptr (std::move (p)))
	{}

	WaitableRef (WaitableRef&&) = default;

	/// Destructor calls ~PtrType if object construction was completed.
	~WaitableRef ()
	{
		if (!this->is_wait_list ())
			this->ref ().~PtrType ();
	}

	/// Called by creator execution domain on finish the object creation.
	/// 
	/// \param p Pointer to the created object.
	void finish_construction (const PtrType& p) noexcept
	{
		WaitableRefBase::finish_construction (make_ptr (p));
	}

	/// Called by creator execution domain on finish the object creation.
	/// 
	/// \param p Pointer to the created object.
	void finish_construction (PtrType&& p) noexcept
	{
		WaitableRefBase::finish_construction (make_ptr (std::move (p)));
	}

	/// Get object pointer.
	/// 
	/// \returns Pointer.
	const PtrType& get () const
	{
		wait_construction ();
		return reinterpret_cast <const PtrType&> (pointer_);
	}

	/// Check for the object constructed
	/// 
	/// \returns `true` if the object construction is complete.
	bool is_constructed () const noexcept
	{
		return !is_wait_list ();
	}

	/// Get object pointer if object is already constructed.
	/// 
	/// Do not use for std::unique_ptr pointer type
	/// 
	/// \returns Pointer or `nullptr`.
	PtrType get_if_constructed () const noexcept
	{
		uintptr_t p = pointer_;
		if (is_wait_list ())
			return nullptr;
		else
			return reinterpret_cast <const PtrType&> (p);
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

private:
	static void* make_ptr (const PtrType& p) noexcept
	{
		void* up = 0;
		new (&up) PtrType (p);
		return up;
	}

	static void* make_ptr (PtrType&& p) noexcept
	{
		void* up = 0;
		new (&up) PtrType (std::move (p));
		return up;
	}

	PtrType& ref () noexcept
	{
		assert (!this->is_wait_list ());
		return reinterpret_cast <PtrType&> (this->pointer_);
	}
};

}
}

#endif
