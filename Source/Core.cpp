#include "core.h"
#include "Heap.h"

#ifdef _WIN32
#include "Windows/MemoryWindows.h"
#else
#error Unknown platform.
#endif

namespace Nirvana {

Memory_ptr g_protection_domain_memory;
Memory_ptr g_default_heap;
HeapFactory_ptr g_heap_factory;

namespace Core {

void initialize ()
{
	Windows::MemoryWindows::initialize ();
	g_protection_domain_memory = Windows::MemoryWindows::_this ();
	Heap::initialize ();
}

void terminate ()
{
	Windows::MemoryWindows::terminate ();
}

}
}