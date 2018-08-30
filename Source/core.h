// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#include <Nirvana.h>
#include <HeapFactory.h>

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

}
}

#endif
