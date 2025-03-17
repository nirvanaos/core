/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#ifndef NIRVANA_CORE_LOCKABLEPTR_H_
#define NIRVANA_CORE_LOCKABLEPTR_H_
#pragma once

#include "TaggedPtr.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class LockablePtrImpl : public std::atomic <uintptr_t>
{
public:
	using IntegralType = uintptr_t;

	LockablePtrImpl ()
	{
		assert (is_lock_free ());
	}

	LockablePtrImpl (uintptr_t ptr) :
		std::atomic <uintptr_t> (ptr)
	{
		assert (is_lock_free ());
	}

	LockablePtrImpl (const LockablePtrImpl&) = delete;
	LockablePtrImpl (LockablePtrImpl&&) = delete;

	LockablePtrImpl& operator = (const LockablePtrImpl&) = delete;
	LockablePtrImpl& operator = (LockablePtrImpl&&) = delete;

	void assign (uintptr_t src, uintptr_t spin_mask_inv) noexcept;
	bool compare_exchange (uintptr_t& cur, uintptr_t to, uintptr_t spin_mask_inv) noexcept;
	uintptr_t lock (uintptr_t spin_mask, uintptr_t inc) noexcept;
};

template <unsigned TAG_BITS, unsigned ALIGN = HEAP_UNIT_CORE>
class LockablePtr
{
public:
	typedef TaggedPtr <TAG_BITS, ALIGN> Ptr;

	LockablePtr () noexcept
	{}

	LockablePtr (Ptr src) noexcept :
		ptr_ (src.ptr_)
	{}

	Ptr load () const noexcept
	{
		return Ptr (ptr_.load (std::memory_order_acquire) & ~LOCK_MASK);
	}

	LockablePtr& operator = (const LockablePtr&) = delete;

	Ptr operator = (Ptr src) noexcept
	{
		ptr_.assign (src.ptr_, ~LOCK_MASK);
		return src;
	}

	bool cas (Ptr from, const Ptr& to) noexcept
	{
		return compare_exchange (from, to);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to) noexcept
	{
		return ptr_.compare_exchange (cur.ptr_, to.ptr_, ~LOCK_MASK);
	}

	Ptr lock () noexcept
	{
		return Ptr (ptr_.lock (LOCK_MASK, LOCK_INC));
	}

	void unlock () noexcept
	{
		ptr_.fetch_sub (LOCK_INC, std::memory_order_release);
	}

	Ptr exchange (const Ptr& to) noexcept
	{
		Ptr cur = load ();

		while (!compare_exchange (cur, to)) {
		}

		return cur;
	}

private:
	static_assert (Ptr::ALIGN_MASK > Ptr::TAG_MASK, "Ptr::ALIGN_MASK > Ptr::TAG_MASK");
	static const uintptr_t LOCK_MASK = Ptr::ALIGN_MASK & ~Ptr::TAG_MASK;
	static const uintptr_t LOCK_INC = 1 << TAG_BITS;

	LockablePtrImpl ptr_;
};

template <class T, unsigned TAG_BITS = 0, unsigned ALIGN = core_object_align (sizeof (T))>
class LockablePtrT : public LockablePtr <TAG_BITS, ALIGN>
{
	typedef LockablePtr <TAG_BITS, ALIGN> Base;
public:
	typedef TaggedPtrT <T, TAG_BITS, ALIGN> Ptr;

	LockablePtrT () noexcept
	{}

	LockablePtrT (Ptr src) noexcept :
		Base (src)
	{}

	Ptr load () const noexcept
	{
		return Ptr (Base::load ());
	}

	Ptr operator = (Ptr src) noexcept
	{
		return Ptr (Base::operator = (src));
	}

	bool cas (const Ptr& from, const Ptr& to) noexcept
	{
		return Base::cas (from, to);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to) noexcept
	{
		return Base::compare_exchange (cur, to);
	}

	Ptr exchange (const Ptr& to) noexcept
	{
		return Ptr (Base::exchange (to));
	}

	Ptr lock () noexcept
	{
		return Ptr (Base::lock ());
	}
};

}
}

#endif
