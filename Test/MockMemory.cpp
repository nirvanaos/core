// Nirvana project
// Tests
// MockMemory class

#include "MockMemory.h"
#include "../Source/core.h"
#include <malloc.h>
#include <memory.h>
#include <Nirvana/real_copy.h>
#include <Nirvana/nlzntz.h>

namespace Nirvana {
namespace Test {

class MockMemory :
	public ::CORBA::Nirvana::ServantStatic <MockMemory, Memory>
{
public:
	static void initialize ()
	{
		Core::g_core_heap = MockMemory::_this ();
	}

	static void terminate ()
	{
		Core::g_core_heap = Memory::_nil ();
	}

	static size_t alignment (size_t size)
	{
		size_t al = clp2 (size);
		if (al < 8)
			al = 8;
		return al;
	}

	// Memory::
	static void* allocate (void* dst, size_t size, long flags)
	{
		if (dst && (flags & Memory::EXACTLY))
			return nullptr;
		size = round_up (size, sizeof (intptr_t));
		void* ret = _aligned_malloc (size, alignment (size));
		if (flags & Memory::ZERO_INIT)
			memset (ret, 0, size);
		return ret;
	}

	static void release (void* p, size_t size)
	{
		_aligned_free (p);
	}

	static void commit (void* ptr, size_t size)
	{}

	static void decommit (void* ptr, size_t size)
	{}

	static void* copy (void* dst, void* src, size_t size, long flags)
	{
		size = round_up (size, sizeof (intptr_t));
		if (!dst)
			dst = allocate (nullptr, size, 0);
		real_copy ((intptr_t*)src, (intptr_t*)src + size / sizeof (intptr_t), (intptr_t*)dst);
		return dst;
	}

	static bool is_readable (const void* p, size_t size)
	{
		return true;
	}

	static bool is_writable (const void* p, size_t size)
	{
		return true;
	}

	static bool is_private (const void* p, size_t size)
	{
		return true;
	}

	static bool is_copy (const void* p1, const void* p2, size_t size)
	{
		return p1 == p2;
	}

	static intptr_t query (const void* p, Memory::QueryParam q)
	{
		return 0;
	}
};

Memory_ptr mock_memory ()
{
	return MockMemory::_this ();
}

}

namespace Core {
namespace Test {

void mock_memory_init ()
{
	g_core_heap = ::Nirvana::Test::MockMemory::_this ();
}

void mock_memory_term ()
{
	g_core_heap = Memory::_nil ();
}

}
}

}
