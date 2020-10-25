/// \file g_memory.cpp
/// The g_memory object implementation.

#include "core.h"
#include <Nirvana/Memory_s.h>
#include <Nirvana/OLF.h>
#include "SynchronizationContext.h"

namespace Nirvana {
namespace Core {

/// Delegates all operations to g_core_heap.
class CoreMemory :
	public ::CORBA::Nirvana::ServantStatic <CoreMemory, Memory>
{
public:
	// Memory::
	static void* allocate (void* dst, size_t size, UWord flags)
	{
		return g_core_heap.allocate (dst, size, flags);
	}

	static void release (void* p, size_t size)
	{
		return g_core_heap.release (p, size);
	}

	static void commit (void* p, size_t size)
	{
		return g_core_heap.commit (p, size);
	}

	static void decommit (void* p, size_t size)
	{
		return g_core_heap.decommit (p, size);
	}

	static void* copy (void* dst, void* src, size_t size, UWord flags)
	{
		return g_core_heap.copy (dst, src, size, flags);
	}

	static bool is_readable (const void* p, size_t size)
	{
		return g_core_heap.is_readable (p, size);
	}

	static bool is_writable (const void* p, size_t size)
	{
		return g_core_heap.is_writable (p, size);
	}

	static bool is_private (const void* p, size_t size)
	{
		return g_core_heap.is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, size_t size)
	{
		return g_core_heap.is_copy (p1, p2, size);
	}

	static intptr_t query (const void* p, MemQuery q)
	{
		return g_core_heap.query (p, q);
	}
};

/// Delegates memory operations dependent on context.
class UserMemory :
	public ::CORBA::Nirvana::ServantStatic <UserMemory, Memory>
{
	static Heap& get_heap ()
	{
		return Thread::current ().synchronization_context ()->memory ();
	}

public:
	// Memory::
	static void* allocate (void* dst, size_t size, UWord flags)
	{
		return get_heap ().allocate (dst, size, flags);
	}

	static void release (void* p, size_t size)
	{
		return get_heap ().release (p, size);
	}

	static void commit (void* p, size_t size)
	{
		return get_heap ().commit (p, size);
	}

	static void decommit (void* p, size_t size)
	{
		return get_heap ().decommit (p, size);
	}

	static void* copy (void* dst, void* src, size_t size, UWord flags)
	{
		return get_heap ().copy (dst, src, size, flags);
	}

	static bool is_readable (const void* p, size_t size)
	{
		return get_heap ().is_readable (p, size);
	}

	static bool is_writable (const void* p, size_t size)
	{
		return get_heap ().is_writable (p, size);
	}

	static bool is_private (const void* p, size_t size)
	{
		return get_heap ().is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, size_t size)
	{
		return get_heap ().is_copy (p1, p2, size);
	}

	static intptr_t query (const void* p, MemQuery q)
	{
		return get_heap ().query (p, q);
	}
};

}

extern const ImportInterfaceT <Memory> g_memory = { OLF_IMPORT_INTERFACE, nullptr, nullptr, STATIC_BRIDGE (Memory, Core::CoreMemory) };

}

NIRVANA_EXPORT (_exp_Nirvana_g_memory, "Nirvana/g_memory", Nirvana::Memory, Nirvana::Core::UserMemory)
