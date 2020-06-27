// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#define NIRVANA_DEBUG_ITERATORS 0

#include <Nirvana/Nirvana.h>
#include <Port/config.h>
#include <memory>

namespace Nirvana {
namespace Core {

extern Memory_ptr g_core_heap;

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
