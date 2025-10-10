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
#ifndef NIRVANA_CORE_TAGGEDPTR_H_
#define NIRVANA_CORE_TAGGEDPTR_H_
#pragma once

#include <Port/config.h>
#include <algorithm>
#include <Nirvana/bitutils.h>

namespace Nirvana {
namespace Core {

constexpr unsigned core_object_align (size_t size)
{
	return std::max ((unsigned)::Nirvana::Core::HEAP_UNIT_CORE, (unsigned)(1 << ::Nirvana::log2_ceil (size)));
}

template <unsigned TAG_BITS, unsigned ALIGN> class AtomicPtr;
template <unsigned TAG_BITS, unsigned ALIGN> class LockablePtr;

template <unsigned TAG_BITS_, unsigned ALIGN>
class TaggedPtr
{
public:
	static const unsigned TAG_BITS = TAG_BITS_;
	static const unsigned ALIGN_BITS = log2_ceil (ALIGN);
	static_assert (ALIGN == 1 << ALIGN_BITS, "Alignment must be power of 2.");
	static const uintptr_t ALIGN_MASK = ALIGN - 1;
	static const uintptr_t TAG_MASK = ~(~(uintptr_t)0 << TAG_BITS);
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
		assert (((uintptr_t)p & ALIGN_MASK) == 0);
		assert ((tag_bits & ~TAG_MASK) == 0);
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
	{
		assert ((val & (ALIGN_MASK & ~TAG_MASK)) == 0);
	}

private:
	friend class AtomicPtr <TAG_BITS, ALIGN>;
	friend class LockablePtr <TAG_BITS, ALIGN>;
	template <unsigned, unsigned>
	friend class TaggedPtr;

	uintptr_t ptr_;
};

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

}
}

#endif
