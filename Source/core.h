// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#include "Heap.h"
#include <memory>

namespace Nirvana {
namespace Core {

class CoreObject
{
public:
	void* operator new (size_t cb)
	{
		return g_core_heap->allocate (nullptr, cb, 0);
	}

	void operator delete (void* p, size_t cb)
	{
		g_core_heap->release (p, cb);
	}
};

template <class T>
class CoreAllocator :
	public std::allocator <T>
{
public:
	void deallocate (T* p, size_t cnt)
	{
		g_core_heap->release (p, cnt * sizeof (T));
	}

	T* allocate (size_t cnt, void* hint = nullptr)
	{
		return (T*)g_core_heap->allocate (0, cnt * sizeof (T), 0);
	}
};

}
}

#endif
