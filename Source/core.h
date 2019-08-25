// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#include <Nirvana/Nirvana.h>
#include <Port/config.h>
#include <Nirvana/HeapFactory_c.h>
#include <Nirvana/Runnable_c.h>

namespace Nirvana {

extern HeapFactory_ptr g_heap_factory;

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

void run_in_neutral_context (Runnable_ptr runnable);

}
}

#endif
