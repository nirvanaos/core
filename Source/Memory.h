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
#ifndef NIRVANA_CORE_MEMORY_H_
#define NIRVANA_CORE_MEMORY_H_
#pragma once

#include "HeapCustom.h"

namespace Nirvana {
namespace Core {

/// Implementation of the Nirvana::Memory interface.
class Memory :
	public CORBA::servant_traits <Nirvana::Memory>::ServantStatic <Memory>
{
public:
	// Memory::
	static void* allocate (void* dst, size_t& size, unsigned flags)
	{
		return Heap::user_heap ().allocate (dst, size, flags);
	}

	static void release (void* p, size_t size)
	{
		return Heap::user_heap ().release (p, size);
	}

	static void commit (void* p, size_t size)
	{
		return Heap::user_heap ().commit (p, size);
	}

	static void decommit (void* p, size_t size)
	{
		return Heap::user_heap ().decommit (p, size);
	}

	static void* copy (void* dst, void* src, size_t& size, unsigned flags)
	{
		return Heap::user_heap ().copy (dst, src, size, flags);
	}

	static bool is_private (const void* p, size_t size)
	{
		return Heap::user_heap ().is_private (p, size);
	}

	static intptr_t query (const void* p, Memory::QueryParam q)
	{
		return Heap::user_heap ().query (p, q);
	}

	static Nirvana::Memory::_ref_type create_heap (size_t granularity)
	{
		return HeapCustom::create (granularity);
	}
};

}
}

#endif
