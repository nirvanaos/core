#include "core.h"
#include "Heap.h"

namespace Nirvana {

HeapFactory_ptr g_heap_factory;

namespace Core {

Memory_ptr g_core_heap;

void initialize ()
{
	Windows::MemoryWindows::initialize ();
	protection_domain_memory () = Windows::MemoryWindows::_this ();
	Heap::initialize ();
}

void terminate ()
{
	Windows::MemoryWindows::terminate ();
}

}
}