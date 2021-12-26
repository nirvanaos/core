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
#ifndef NIRVANA_CORE_MEMORY_H_
#define NIRVANA_CORE_MEMORY_H_

#include <CORBA/Server.h>
#include <Memory_s.h>
#include "ExecDomain.h"
#include "user_memory.h"

namespace Nirvana {
namespace Core {

/// Implementation of the Nirvana::Memory interface.
class Memory :
	public CORBA::servant_traits <Nirvana::Memory>::ServantStatic <Memory>
{
public:
	// Memory::
	static void* allocate (void* dst, size_t size, unsigned flags)
	{
		return heap ().allocate (dst, size, flags);
	}

	static void release (void* p, size_t size)
	{
		return heap ().release (p, size);
	}

	static void commit (void* p, size_t size)
	{
		return heap ().commit (p, size);
	}

	static void decommit (void* p, size_t size)
	{
		return heap ().decommit (p, size);
	}

	static void* copy (void* dst, void* src, size_t size, unsigned flags)
	{
		return heap ().copy (dst, src, size, flags);
	}

	static bool is_readable (const void* p, size_t size)
	{
		return heap ().is_readable (p, size);
	}

	static bool is_writable (const void* p, size_t size)
	{
		return heap ().is_writable (p, size);
	}

	static bool is_private (const void* p, size_t size)
	{
		return heap ().is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, size_t size)
	{
		return heap ().is_copy (p1, p2, size);
	}

	static intptr_t query (const void* p, Memory::QueryParam q)
	{
		return heap ().query (p, q);
	}

private:
	static Heap& heap ()
	{
#if defined (NIRVANA_CORE_TEST)
		Thread* th = Thread::current_ptr ();
		if (th) {
			ExecDomain* ed = th->exec_domain ();
			if (ed)
				return ed->sync_context ().memory ();
		}
		// Fallback to g_core_heap
		return g_core_heap.object ();
#else
		return user_memory ();
#endif
	}
};

}
}

#endif
