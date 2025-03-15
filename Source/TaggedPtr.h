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
#ifndef NIRVANA_CORE_TAGGEDPTR_H_
#define NIRVANA_CORE_TAGGEDPTR_H_
#pragma once

#include <Port/config.h>
#include "BackOff.h"
#include <Nirvana/bitutils.h>
#include <atomic>
#include <algorithm>

#ifndef ATOMIC_POINTER_LOCK_FREE
#error Atomic pointer is required.
#endif

namespace Nirvana {
namespace Core {

constexpr unsigned core_object_align (size_t size)
{
	return std::max ((unsigned)::Nirvana::Core::HEAP_UNIT_CORE, (unsigned)(1 << ::Nirvana::log2_ceil (size)));
}

template <unsigned TAG_BITS, unsigned ALIGN> class AtomicPtr;
template <unsigned TAG_BITS, unsigned ALIGN> class LockablePtr;

template <unsigned TAG_BITS, unsigned ALIGN = HEAP_UNIT_CORE>
class TaggedPtr
{
	static_assert (ALIGN == 1 << log2_ceil (ALIGN), "Alignment must be power of 2.");
public:
	static const uintptr_t ALIGN_MASK = ALIGN - 1;
	static const uintptr_t TAG_MASK = ~(~0u << TAG_BITS);
	static_assert (ALIGN_MASK >= TAG_MASK, "ALIGN_MASK >= TAG_MASK");
	typedef AtomicPtr <TAG_BITS, ALIGN> Atomic;
	typedef LockablePtr <TAG_BITS, ALIGN> Lockable;

	TaggedPtr () noexcept
	{}

	TaggedPtr (void* p) noexcept
	{
		assert (!((uintptr_t)p & ALIGN_MASK));
		ptr_ = (uintptr_t)p;
	}

	TaggedPtr (void* p, uintptr_t tag_bits) noexcept
	{
		assert (!((uintptr_t)p & ALIGN_MASK));
		assert (!(tag_bits & ~TAG_MASK));
		ptr_ = (uintptr_t)p | tag_bits;
	}

	template <unsigned A>
	TaggedPtr (const TaggedPtr <TAG_BITS, A>& src) noexcept :
		ptr_ (src.ptr_)
	{
		static_assert (A > ALIGN_MASK, "Alignment decreasing.");
	}

	template <unsigned A>
	TaggedPtr <TAG_BITS, ALIGN>& operator = (const TaggedPtr <TAG_BITS, A>& src) noexcept
	{
		static_assert (A > ALIGN_MASK, "Alignment decreasing.");
		ptr_ = src.ptr_;
		return *this;
	}

	void* operator = (void* p) noexcept
	{
		assert (!((uintptr_t)p & ALIGN_MASK));
		ptr_ = (uintptr_t)p;
		return p;
	}

	operator void* () const noexcept
	{
		return (void*)(ptr_ & ~TAG_MASK);
	}

	unsigned tag_bits () const noexcept
	{
		return (unsigned)ptr_ & TAG_MASK;
	}

	TaggedPtr <TAG_BITS, ALIGN> untagged () const noexcept
	{
		return TaggedPtr <TAG_BITS, ALIGN> (ptr_ & ~TAG_MASK);
	}

	template <unsigned A>
	bool operator == (const TaggedPtr <TAG_BITS, A>& rhs) const noexcept
	{
		return ptr_ == rhs.ptr_;
	}

private:
	explicit TaggedPtr (uintptr_t val) noexcept :
		ptr_ (val)
	{}

private:
	friend class AtomicPtr <TAG_BITS, ALIGN>;
	friend class LockablePtr <TAG_BITS, ALIGN>;
	template <unsigned, unsigned>
	friend class TaggedPtr;

	uintptr_t ptr_;
};

template <unsigned TAG_BITS, unsigned ALIGN = HEAP_UNIT_CORE>
class AtomicPtr
{
public:
	typedef TaggedPtr <TAG_BITS, ALIGN> Ptr;

	AtomicPtr () noexcept
	{
		assert (ptr_.is_lock_free ());
	}

	AtomicPtr (Ptr src) noexcept :
		ptr_ (src.ptr_)
	{
		assert (ptr_.is_lock_free ());
	}

	Ptr load () const noexcept
	{
		return Ptr (ptr_.load (std::memory_order_acquire));
	}

	Ptr operator = (Ptr src) noexcept
	{
		ptr_.store (src.ptr_, std::memory_order_release);
		return src;
	}

	bool cas (Ptr from, const Ptr& to) noexcept
	{
		return compare_exchange (from, to);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to) noexcept
	{
		return ptr_.compare_exchange_weak (cur.ptr_, to.ptr_);
	}

	Ptr exchange (const Ptr& to) noexcept
	{
		return Ptr (ptr_.exchange (to.ptr_));
	}

private:
	volatile std::atomic <uintptr_t> ptr_;
};

template <unsigned TAG_BITS, unsigned ALIGN = HEAP_UNIT_CORE>
class LockablePtr
{
public:
	typedef TaggedPtr <TAG_BITS, ALIGN> Ptr;

	LockablePtr () noexcept
	{
		assert (ptr_.is_lock_free ());
	}

	LockablePtr (Ptr src) noexcept :
		ptr_ (src.ptr_)
	{
		assert (ptr_.is_lock_free ());
	}

	LockablePtr (const LockablePtr&) = delete;

	Ptr load () const noexcept
	{
		return Ptr (ptr_.load (std::memory_order_acquire) & ~SPIN_MASK);
	}

	LockablePtr& operator = (const LockablePtr&) = delete;

	Ptr operator = (Ptr src) noexcept
	{
		assert ((src.ptr_ & SPIN_MASK) == 0);
		uintptr_t p = ptr_.load (std::memory_order_relaxed) & ~SPIN_MASK;
		while (!ptr_.compare_exchange_weak (p, src.ptr_)) {
			p &= ~SPIN_MASK;
		}
		return src;
	}

	bool cas (Ptr from, const Ptr& to) noexcept
	{
		return compare_exchange (from, to);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to) noexcept;

	Ptr lock () noexcept;

	void unlock () noexcept
	{
		ptr_.fetch_sub (Ptr::TAG_MASK + 1, std::memory_order_release);
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
	static const uintptr_t SPIN_MASK = Ptr::ALIGN_MASK & ~Ptr::TAG_MASK;

	volatile std::atomic <uintptr_t> ptr_;
};

template <unsigned TAG_BITS, unsigned ALIGN>
bool LockablePtr <TAG_BITS, ALIGN>::compare_exchange (Ptr& cur, const Ptr& to) noexcept
{
	uintptr_t tcur = cur.ptr_;
	assert ((tcur & SPIN_MASK) == 0);
	assert ((to.ptr_ & SPIN_MASK) == 0);
	while (!ptr_.compare_exchange_weak (tcur, to.ptr_)) {
		tcur &= ~SPIN_MASK;
		if (tcur != cur.ptr_) {
			cur.ptr_ = tcur;
			return false;
		}
	}
	return true;
}

template <unsigned TAG_BITS, unsigned ALIGN>
typename LockablePtr <TAG_BITS, ALIGN>::Ptr LockablePtr <TAG_BITS, ALIGN>::lock () noexcept
{
	for (BackOff bo; true; bo ()) {
		uintptr_t cur = ptr_.load (std::memory_order_acquire);
		while ((cur & SPIN_MASK) != SPIN_MASK) {
			if (ptr_.compare_exchange_weak (cur, cur + Ptr::TAG_MASK + 1))
				return Ptr (cur & ~SPIN_MASK);
		}
	}
}

template <class T, unsigned TAG_BITS, unsigned ALIGN> class AtomicPtrT;
template <class T, unsigned TAG_BITS, unsigned ALIGN> class LockablePtrT;

template <class T, unsigned TAG_BITS, unsigned ALIGN = core_object_align (sizeof (T))>
class TaggedPtrT : public TaggedPtr <TAG_BITS, ALIGN>
{
	typedef TaggedPtr <TAG_BITS, ALIGN> Base;
public:
	typedef AtomicPtrT <T, TAG_BITS, ALIGN> Atomic;
	typedef LockablePtrT <T, TAG_BITS, ALIGN> Lockable;

	TaggedPtrT () noexcept
	{}

	TaggedPtrT (T* p) noexcept :
		Base (p)
	{}

	TaggedPtrT (T* p, unsigned tag_bits) noexcept :
		Base (p, tag_bits)
	{}

	template <unsigned A>
	TaggedPtrT (const TaggedPtrT <T, TAG_BITS, A>& src) noexcept :
		Base (src)
	{}

	T* operator = (T* p) noexcept
	{
		return (T*)Base::operator = (p);
	}

	template <unsigned A>
	TaggedPtrT <T, TAG_BITS, ALIGN>& operator = (const TaggedPtrT <T, TAG_BITS, A>& src)
		noexcept
	{
		Base::operator = (src);
		return *this;
	}

	operator T* () const noexcept
	{
		return (T*)Base::operator void* ();
	}

	operator bool () const noexcept
	{
		return Base::operator void* ();
	}

	T* operator -> () const noexcept
	{
		return operator T* ();
	}

	TaggedPtrT <T, TAG_BITS, ALIGN> untagged () const noexcept
	{
		return TaggedPtrT <T, TAG_BITS, ALIGN> (Base::untagged ());
	}

private:
	friend class AtomicPtrT <T, TAG_BITS, ALIGN>;
	friend class LockablePtrT <T, TAG_BITS, ALIGN>;

	explicit TaggedPtrT (const Base& src) :
		Base (src)
	{}
};

template <class T, unsigned TAG_BITS, unsigned ALIGN = core_object_align (sizeof (T))>
class AtomicPtrT : public AtomicPtr <TAG_BITS, ALIGN>
{
	typedef AtomicPtr <TAG_BITS, ALIGN> Base;
public:
	typedef TaggedPtrT <T, TAG_BITS, ALIGN> Ptr;

	AtomicPtrT () noexcept
	{}

	AtomicPtrT (Ptr src) noexcept :
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
