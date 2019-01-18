// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#include <Nirvana/Nirvana.h>
#include <Nirvana/HeapFactory.h>
#include "Scheduler.h"
#include <memory>

namespace Nirvana {

extern HeapFactory_ptr g_heap_factory;

namespace Core {

extern Memory_ptr g_protection_domain_memory;
extern Memory_ptr g_core_heap;
extern Scheduler_ptr g_scheduler;

inline Memory_ptr protection_domain_memory ()
{
	return g_protection_domain_memory;
}

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

}
}

#endif
