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
#ifndef NIRVANA_CORE_HEAPALLOCATOR_H_
#define NIRVANA_CORE_HEAPALLOCATOR_H_
#pragma once

#include "Heap.h"

namespace Nirvana {
namespace Core {

/// Allocates memory from the specific heap.
template <class T>
class HeapAllocator :
	public std::allocator <T>
{
public:
	DEFINE_ALLOCATOR (HeapAllocator);

	// Some containers require default constructor for allocator.
	HeapAllocator () NIRVANA_NOEXCEPT
	{
	}

	HeapAllocator (Heap& heap) NIRVANA_NOEXCEPT :
		heap_ (&heap)
	{
	}

	HeapAllocator (const HeapAllocator& src) :
		heap_ (src.heap_)
	{
	}

	template <class U>
	HeapAllocator (const HeapAllocator <U>& src) :
		heap_ (src.heap_)
	{
	}

	HeapAllocator& operator = (const HeapAllocator& src)
	{
		heap_ = src.heap_;
		return *this;
	}

	template <class U>
	HeapAllocator& operator = (const HeapAllocator <U>& src)
	{
		heap_ = src.heap_;
		return *this;
	}

	void deallocate (T* p, size_t cnt) const
	{
		heap_->release (p, cnt * sizeof (T));
	}

	T* allocate (size_t cnt) const
	{
		size_t cb = cnt * sizeof (T);
		return (T*)heap_->allocate (nullptr, cb, 0);
	}

private:
	template <class U> friend class HeapAllocator;
	Ref <Heap> heap_;
};

template <class T, class U>
bool operator == (const HeapAllocator <T>& l, const HeapAllocator <U>& r)
{
	return l.heap_ == r.heap_;
}

template <class T, class U>
bool operator != (const HeapAllocator <T>& l, const HeapAllocator <U>& r)
{
	return l.heap_ != r.heap_;
}

}
}

#endif