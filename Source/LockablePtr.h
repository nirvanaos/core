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
#include <type_traits>

#ifndef ATOMIC_POINTER_LOCK_FREE
#error Atomic pointer is required.
#endif

namespace Nirvana {
namespace Core {

class LockablePtrImpl
{
protected:
	static_assert (sizeof (void*) == 8 || sizeof (void*) == 4 || sizeof (void*) == 2, "Unknown platform address size");

	// I think, 8 bit lock count is enough
	static const unsigned SURELY_ENOUGH_LOCK_BITS = 8;

	using IntegralType = std::conditional <sizeof (void*) == 8, uintptr_t,
		std::conditional <sizeof (void*) == 4,
#if ATOMIC_LLONG_LOCK_FREE
		std::conditional <USE_LOCKABLE_PTR_64, uint64_t, uintptr_t>::type
#else
		uintptr_t
#endif
		,
#if ATOMIC_LONG_LOCK_FREE
		uint32_t
#else
		uintptr_t
#endif
		>::type>::type;

	// Address space properties, see Port/config.h
	static const unsigned ADDRESS_BITS = ADDRESS_BITS_USED;
	static const uintptr_t UNUSED_BITS_VAL = ADDRESS_UNUSED_BITS_VAL;

	static const unsigned UNUSED_BITS = sizeof (void*) * 8 - ADDRESS_BITS;
	static const uintptr_t UNUSED_BITS_MASK = ~(uintptr_t)0 << ADDRESS_BITS;

	static const unsigned LOCK_BITS = (sizeof (IntegralType) - sizeof (void*)) * 8 + UNUSED_BITS;

public:
	static const bool USE_SHIFT = LOCK_BITS < SURELY_ENOUGH_LOCK_BITS;

protected:
	LockablePtrImpl () noexcept
	{
		assert (ptr_.is_lock_free ());
	}

	LockablePtrImpl (uintptr_t ptr) noexcept :
		ptr_ (ptr)
	{
		assert (ptr_.is_lock_free ());
	}

	LockablePtrImpl (const LockablePtrImpl&) = delete;
	LockablePtrImpl (LockablePtrImpl&&) = delete;

	LockablePtrImpl& operator = (const LockablePtrImpl&) = delete;
	LockablePtrImpl& operator = (LockablePtrImpl&&) = delete;

	void assign (uintptr_t src, IntegralType lock_mask_inv) noexcept;
	bool compare_exchange (uintptr_t& cur, uintptr_t to, IntegralType lock_mask_inv) noexcept;
	uintptr_t lock (IntegralType mask, IntegralType inc) noexcept;

	void unlock (IntegralType inc) noexcept
	{
		ptr_.fetch_sub (inc, std::memory_order_release);
	}

	uintptr_t load (IntegralType lock_mask_inv) noexcept
	{
		return (uintptr_t)(ptr_.load (std::memory_order_acquire) & lock_mask_inv);
	}

private:
	std::atomic <IntegralType> ptr_;
};

template <unsigned TAG_BITS, unsigned ALIGN>
class LockablePtr : public LockablePtrImpl
{
public:
	typedef TaggedPtr <TAG_BITS, ALIGN> Ptr;
	static_assert (!USE_SHIFT || Ptr::ALIGN_MASK > Ptr::TAG_MASK, "Ptr::ALIGN_MASK > Ptr::TAG_MASK");

	LockablePtr () noexcept
	{}

	LockablePtr (Ptr src) noexcept :
		LockablePtrImpl (to_lockable (src))
	{}

	Ptr load () noexcept
	{
		return from_lockable (LockablePtrImpl::load (~LOCK_MASK));
	}

	Ptr operator = (Ptr src) noexcept
	{
		assign (to_lockable (src), ~LOCK_MASK);
		return src;
	}

	bool cas (const Ptr& from, const Ptr& to) noexcept
	{
		uintptr_t lcur = to_lockable (from), lto = to_lockable (to);
		return LockablePtrImpl::compare_exchange (lcur, lto, ~LOCK_MASK);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to) noexcept
	{
		uintptr_t lcur = to_lockable (cur), lto = to_lockable (to);
		bool ret = LockablePtrImpl::compare_exchange (lcur, lto, ~LOCK_MASK);
		if (!ret)
			cur = from_lockable (lcur);
		return ret;
	}

	Ptr lock () noexcept
	{
		return from_lockable (LockablePtrImpl::lock (LOCK_MASK, LOCK_INC));
	}

	void unlock () noexcept
	{
		LockablePtrImpl::unlock (LOCK_INC);
	}

	Ptr exchange (const Ptr& to) noexcept
	{
		uintptr_t lto = to_lockable (to);
		uintptr_t cur = LockablePtrImpl::load (~LOCK_MASK);
		while (!LockablePtrImpl::compare_exchange (cur, lto, ~LOCK_MASK))
			;

		return from_lockable (cur);
	}

private:
	static const unsigned SHIFT_BITS = USE_SHIFT ? (Ptr::ALIGN_BITS - Ptr::TAG_BITS) : 0;
	static const unsigned LOCK_BITS = USE_SHIFT ?
		(LockablePtrImpl::LOCK_BITS + SHIFT_BITS) : LockablePtrImpl::LOCK_BITS;

	static const unsigned LOCK_OFFSET = sizeof (IntegralType) * 8 - LOCK_BITS;
	static const IntegralType LOCK_MASK = ~(IntegralType)0 << LOCK_OFFSET;
	static const IntegralType LOCK_INC = (IntegralType)1 << LOCK_OFFSET;

	static uintptr_t to_lockable (Ptr src) noexcept
	{
		assert ((src.ptr_ & (Ptr::ALIGN_MASK & ~Ptr::TAG_MASK)) == 0);
		assert ((src.ptr_ & UNUSED_BITS_MASK) == UNUSED_BITS_VAL);
		uintptr_t u = src.ptr_ & ~UNUSED_BITS_VAL;
		if (SHIFT_BITS) {
			if (Ptr::TAG_MASK)
				u = ((u & ~Ptr::TAG_MASK) >> SHIFT_BITS) | (u & Ptr::TAG_MASK);
			else
				u >>= SHIFT_BITS;
		}
		assert ((u & LOCK_MASK) == 0);
		return u;
	}

	static Ptr from_lockable (uintptr_t u) noexcept
	{
		assert ((u & LOCK_MASK) == 0);
		if (SHIFT_BITS) {
			if (Ptr::TAG_MASK)
				u = ((u & ~Ptr::TAG_MASK) << SHIFT_BITS) | (u & Ptr::TAG_MASK);
			else
				u <<= SHIFT_BITS;
		}
		u |= UNUSED_BITS_VAL;
		return Ptr (u);
	}

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

	Ptr load () noexcept
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
