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
#ifndef NIRVANA_CORE_COREALLOCATOR_H_
#define NIRVANA_CORE_COREALLOCATOR_H_
#pragma once

#include "Heap.h"
#include <memory>

namespace Nirvana {
namespace Core {

/// Allocates memory from the core heap.
/// Currently unused. Just for case.
template <class T>
class CoreAllocator :
	public std::allocator <T>
{
public:
	DEFINE_ALLOCATOR (CoreAllocator);

	static void deallocate (T* p, size_t cnt)
	{
		Heap::core_heap ().release (p, cnt * sizeof (T));
	}

	static T* allocate (size_t cnt)
	{
		size_t cb = cnt * sizeof (T);
		return (T*)Heap::core_heap ().allocate (nullptr, cb, 0);
	}

};

}
}

#endif
