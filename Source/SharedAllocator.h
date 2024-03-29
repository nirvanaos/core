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
#ifndef NIRVANA_CORE_SHAREDALLOCATOR_H_
#define NIRVANA_CORE_SHAREDALLOCATOR_H_
#pragma once

#include "Heap.h"
#include <memory>

namespace Nirvana {
namespace Core {

/// Allocate from the shared heap.
template <class T>
class SharedAllocator :
	public std::allocator <T>
{
public:
	DEFINE_ALLOCATOR (SharedAllocator);

	static void deallocate (T* p, size_t cnt)
	{
		Heap::shared_heap ().release (p, cnt * sizeof (T));
	}

	static T* allocate (size_t cnt, void* hint = nullptr, unsigned flags = 0)
	{
		size_t cb = cnt * sizeof (T);
		return (T*)Heap::shared_heap ().allocate (hint, cb, flags);
	}
};

typedef std::basic_string <char, std::char_traits <char>, SharedAllocator <char> > SharedString;

}
}

#endif
