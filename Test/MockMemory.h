// Nirvana project
// Core
// MockMemory class

#include "../Interface/Memory.h"
#include <malloc.h>
#include <memory.h>
#include <math.h>
#include <real_copy.h>
#include <nlzntz.h>
#include "../Source/core.h"

namespace Nirvana {
namespace Test {

using ::Nirvana::Memory;

class MockMemory :
	public ::CORBA::Nirvana::ServantStatic <MockMemory, ::Nirvana::Memory>
{
public:
	static void initialize ()
	{
		::Nirvana::Core::g_core_heap = _this ();
	}

	static void terminate ()
	{
		::Nirvana::Core::g_core_heap = Memory::_nil ();
	}

	static size_t alignment (size_t size)
	{
		size_t al = 1 << (int)ceil (log2 (size));
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

}
}