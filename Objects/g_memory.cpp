/// \file g_memory.cpp
/// The g_memory object implementation.

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
#include "core.h"
#include <CORBA/Server.h>
#include <Nirvana/Memory_s.h>
#include <Nirvana/OLF.h>
#include "user_memory.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

/// Delegates all operations to g_core_heap->
class CoreMemory :
	public ::CORBA::Nirvana::ServantStatic <CoreMemory, Memory>
{
public:
	// Memory::
	static void* allocate (void* dst, size_t size, UWord flags)
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

	static void* copy (void* dst, void* src, size_t size, UWord flags)
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

	static intptr_t query (const void* p, Memory::Query q)
	{
		return heap ().query (p, q);
	}

private:
	static Heap& heap ()
	{
		Thread* th = Thread::current_ptr ();
		if (th) {
			ExecDomain* ed = th->exec_domain ();
			if (ed) {
				SyncContext* sc = ed->sync_context ();
				if (sc)
					return sc->memory ();
			}
		}
		return g_core_heap.object ();
	}
};

/// Delegates memory operations dependent on context.
class UserMemory :
	public ::CORBA::Nirvana::ServantStatic <UserMemory, Memory>
{
public:
	// Memory::
	static void* allocate (void* dst, size_t size, UWord flags)
	{
		return user_memory ().allocate (dst, size, flags);
	}

	static void release (void* p, size_t size)
	{
		return user_memory ().release (p, size);
	}

	static void commit (void* p, size_t size)
	{
		return user_memory ().commit (p, size);
	}

	static void decommit (void* p, size_t size)
	{
		return user_memory ().decommit (p, size);
	}

	static void* copy (void* dst, void* src, size_t size, UWord flags)
	{
		return user_memory ().copy (dst, src, size, flags);
	}

	static bool is_readable (const void* p, size_t size)
	{
		return user_memory ().is_readable (p, size);
	}

	static bool is_writable (const void* p, size_t size)
	{
		return user_memory ().is_writable (p, size);
	}

	static bool is_private (const void* p, size_t size)
	{
		return user_memory ().is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, size_t size)
	{
		return user_memory ().is_copy (p1, p2, size);
	}

	static intptr_t query (const void* p, Memory::Query q)
	{
		return user_memory ().query (p, q);
	}
};

}

extern const ImportInterfaceT <Memory> g_memory = { OLF_IMPORT_INTERFACE, nullptr, nullptr, NIRVANA_STATIC_BRIDGE (Memory, Core::CoreMemory) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_memory, "Nirvana/g_memory", Nirvana::Memory, Nirvana::Core::UserMemory)
