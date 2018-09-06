#include "core.h"
#include "Heap.h"
#include "SysDomain.h"
#include "ProtDomain.h"

namespace Nirvana {

HeapFactory_ptr g_heap_factory;

namespace Core {

Memory_ptr g_core_heap = Memory_ptr::nil ();

void initialize ()
{
	Heap::initialize ();
	SysDomain::initialize ();
	ProtDomain::initialize ();
}

void terminate ()
{
	ProtDomain::terminate ();
	SysDomain::terminate ();
	Heap::terminate ();
}

}
}
