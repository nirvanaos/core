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
#ifndef NIRVANA_CORE_HEAPUSER_H_
#define NIRVANA_CORE_HEAPUSER_H_
#pragma once

#include "SharedObject.h"

namespace Nirvana {
namespace Core {

/// User-mode heap.
class HeapUser :
	public Heap,
	public SharedObject
{
public:
	HeapUser (size_t allocation_unit = HEAP_UNIT_DEFAULT) :
		Heap (allocation_unit)
	{}

	~HeapUser ()
	{
		cleanup ();
		// TODO: Log message if memory leaks detected.
	}

	/// \brief Releases all memory.
	/// \returns `true` if no memory leaks.
	bool cleanup ();

	/* Unused. Kept just for case.
	HeapUser& operator = (HeapUser&& other) noexcept
	{
		cleanup ();
		allocation_unit_ = other.allocation_unit_;
		part_list_ = other.part_list_;
		other.part_list_ = nullptr;
		block_list_ = std::move (other.block_list_);
		return *this;
	}
	*/
};

inline bool Heap::initialize () noexcept
{
	if (!Port::Memory::initialize ())
		return false;
	core_heap_.construct ();
	if (sizeof (void*) > 2)
		shared_heap_.construct ();
	return true;
}

inline void Heap::terminate () noexcept
{
	if (sizeof (void*) > 2)
		shared_heap_.destruct ();
	core_heap_.destruct ();
	Port::Memory::terminate ();
}

inline Heap& Heap::core_heap () noexcept
{
	return core_heap_;
}

inline Heap& Heap::shared_heap () noexcept
{
	if (sizeof (void*) > 2)
		return shared_heap_;
	else
		return core_heap_;
}

}
}

#endif
