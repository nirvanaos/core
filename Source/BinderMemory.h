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
#ifndef NIRVANA_CORE_BINDERMEMORY_H_
#define NIRVANA_CORE_BINDERMEMORY_H_
#pragma once

#include "Heap.h"
#include "StaticallyAllocated.h"
#include "SharedAllocator.h"

namespace Nirvana {
namespace Core {

class BinderMemory
{
	// The Binder is high-loaded system service and should have own heap.
	// Use shared only on systems with low resources.
	static const bool USE_SHARED_MEMORY = (sizeof (void*) <= 2);

public:
	template <class T>
	class OwnAllocator : public std::allocator <T>
	{
	public:
		DEFINE_ALLOCATOR (OwnAllocator);

		static void deallocate (T* p, size_t cnt)
		{
			heap_->release (p, cnt * sizeof (T));
		}

		static T* allocate (size_t cnt)
		{
			size_t cb = cnt * sizeof (T);
			return (T*)heap_->allocate (nullptr, cb, 0);
		}
	};

	template <class T>
	using Allocator = std::conditional_t <USE_SHARED_MEMORY,
		SharedAllocator <T>, OwnAllocator <T> >;

	static void initialize ()
	{
		if (!USE_SHARED_MEMORY)
			heap_.construct ();
	}

	static void terminate () noexcept
	{
		if (!USE_SHARED_MEMORY)
			heap_.destruct ();
	}

	static Heap& heap () noexcept
	{
		if (!USE_SHARED_MEMORY)
			return Heap::shared_heap ();
		else
			return heap_;
	}

private:
	static StaticallyAllocated <ImplStatic <Heap> > heap_;
};

}
}

#endif
