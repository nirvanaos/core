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
#ifndef NIRVANA_CORE_ATOMICPTR_H_
#define NIRVANA_CORE_ATOMICPTR_H_
#pragma once

#include "TaggedPtr.h"
#include <atomic>

namespace Nirvana {
namespace Core {

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
	std::atomic <uintptr_t> ptr_;
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

}
}

#endif
