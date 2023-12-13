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
#ifndef NIRVANA_CORE_WAITLIST_H_
#define NIRVANA_CORE_WAITLIST_H_
#pragma once

#include "UserObject.h"
#include "CoreInterface.h"
#include "EventSync.h"
#include <exception>

namespace Nirvana {
namespace Core {

class ExecDomain;

class WaitListBase;
template <typename> class WaitableRef;
template <typename> class WaitList;

template <class Base> 
using WaitListImpl = ImplDynamicSync <Base>;

template <class List>
using WaitListRef = Ref <WaitListImpl <List> >;

class WaitableRefBase
{
	WaitableRefBase (const WaitableRefBase&) = delete;
	WaitableRefBase& operator = (const WaitableRefBase&) = delete;
	WaitableRefBase (WaitableRefBase&&) = delete;

	template <typename> friend class WaitList;
public:
	/// \returns `true` if object creation is not completed.
	bool is_wait_list () const noexcept
	{
		return is_wait_list (pointer_);
	}

	bool initialize (TimeBase::TimeT deadline);

	bool initialized () const noexcept
	{
		return pointer_ != 0;
	}

protected:
	WaitableRefBase () noexcept :
		pointer_ (0)
	{}

	WaitableRefBase (void* p) noexcept :
		pointer_ ((uintptr_t)p)
	{
		assert (!is_wait_list ());
	}

	WaitableRefBase (TimeBase::TimeT deadline) :
		pointer_ (0)
	{
		verify (initialize (deadline));
	}

	~WaitableRefBase ()
	{}

	const uintptr_t& wait_construction () const;
	WaitListRef <WaitListBase> finish_construction (void* p) noexcept;

	void reset () noexcept;

	WaitListRef <WaitListBase> wait_list () const noexcept
	{
		assert (is_wait_list ());
		return reinterpret_cast <WaitListImpl <WaitListBase>*> (pointer_ & ~1);
	}

private:
	static bool is_wait_list (uintptr_t pointer) noexcept
	{
		return pointer & 1;
	}

	WaitListRef <WaitListBase> detach_list () noexcept;

protected:
	uintptr_t pointer_;
};

// Synchronous wait list.
// May be used only in synchronization domain.
class WaitListBase : public UserObject
{
public:
	~WaitListBase ()
	{
		assert (!worker_);
		assert (!result_);
	}

	void on_exception () noexcept;

	bool finished () const noexcept
	{
		return !worker_;
	}

protected:
	/// Constructor.
	/// 
	/// \param deadline Maximal deadline of the object creation.
	/// \throw BAD_OPERATION if called out of synchronization domain.
	WaitListBase (TimeBase::TimeT deadline, WaitableRefBase* ref);

	uintptr_t& wait ();
	void finish (void* result) noexcept;

private:
	friend class WaitableRefBase;

	void on_reset_ref () noexcept
	{
		ref_ = nullptr;
	}

protected:
	ExecDomain* worker_;
	uintptr_t result_;
	WaitableRefBase* ref_;
	EventSync event_;
	DeadlineTime worker_deadline_;
	std::exception_ptr exception_;
};

template <typename PtrType>
class WaitList : public WaitListBase
{
	static_assert (sizeof (PtrType) == sizeof (uintptr_t), "Invalid pointer type");

public:
	~WaitList ()
	{
		if (result_) {
			make_ref (result_).~PtrType ();
#ifndef NDEBUG
			result_ = 0; // For assert() in WaitListBase destructor
#endif
		}
	}

	PtrType wait ()
	{
		return make_ref (WaitListBase::wait ());
	}

	/// Called by creator execution domain on finish the object creation.
	/// 
	/// \param p Pointer to the created object.
	void finish_construction (PtrType&& p) noexcept
	{
		if (!finished ()) {
			finish_construction_ref (p);
			finish (make_ptr (std::move (p)));
		}
	}

	/// Called by creator execution domain on finish the object creation.
	/// 
	/// \param p Pointer to the created object.
	void finish_construction (const PtrType& p) noexcept
	{
		if (!finished ()) {
			finish_construction_ref (p);
			finish (make_ptr (p));
		}
	}

private:
	friend class WaitableRef <PtrType>;

	static void* make_ptr (const PtrType& p)
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

	static PtrType& make_ref (uintptr_t& p) noexcept
	{
		return reinterpret_cast <PtrType&> (p);
	}

	static const PtrType& make_ref (const uintptr_t& p) noexcept
	{
		return reinterpret_cast <const PtrType&> (p);
	}

	void finish_construction_ref (const PtrType& p) noexcept
	{
		if (ref_)
			WaitListRef <WaitList>::cast (static_cast <WaitListBase*> (ref_->finish_construction (make_ptr (p))));
	}

};

}
}

#endif
