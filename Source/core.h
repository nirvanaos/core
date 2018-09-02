// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#include <Nirvana.h>
#include <HeapFactory.h>
#include <RefCountBase.h>
#include <memory>
#include <random>

#ifdef _WIN32
#include "Windows/MemoryWindows.h"
namespace Nirvana {
namespace Core {
typedef Windows::MemoryWindows MemoryManager;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {

extern HeapFactory_ptr g_heap_factory;

namespace Core {

void initialize ();
void terminate ();

inline Memory_ptr protection_domain_memory ()
{
	return MemoryManager::_this ();
}

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

class ProtDomain;

/// Current protection domain.
extern ProtDomain* g_protection_domain;

/// Random-number generator.
typedef std::mt19937 RandomGen;

}
}

#endif
