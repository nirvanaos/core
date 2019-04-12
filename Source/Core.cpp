#include "core.h"
#include "Heap.h"
#include <Port/Scheduler.h>

namespace Nirvana {

HeapFactory_ptr g_heap_factory;

namespace Core {

Memory_ptr g_core_heap = Memory_ptr::nil ();

inline Scheduler_ptr scheduler ()
{
	return Port::Scheduler::singleton ();
}

}
}
