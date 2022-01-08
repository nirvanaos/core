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
#ifndef NIRVANA_CORE_ALLOCATORS_H_
#define NIRVANA_CORE_ALLOCATORS_H_

#include "Heap.h"
#include "Stack.h"

namespace Nirvana {
namespace Core {

template <unsigned SIZE>
class FreeList :
	private Stack <StackElem, SIZE>
{
public:
	void push (void* p)
	{
		Stack <StackElem, SIZE>::push (*(StackElem*)p);
	}

	void* pop ()
	{
		return Stack <StackElem, SIZE>::pop ();
	}
};

template <class T>
class AllocatorFixedSize :
	public std::allocator <T>
{
public:
	void deallocate (T* p, size_t cnt)
	{
		assert (1 == cnt);
		free_list_.push (p);
	}

	T* allocate (size_t cnt, void* hint = nullptr)
	{
		assert (1 == cnt);
		T* p = (T*)free_list_.pop ();
		if (!p)
			p = (T*)g_core_heap->allocate (0, sizeof (T), 0);
		return p;
	}

private:
	FreeList <CORE_OBJECT_ALIGN (T)> free_list_;
};

}
}

#endif
