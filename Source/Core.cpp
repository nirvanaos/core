#include "core.h"
#include "Heap.h"

namespace Nirvana {

HeapFactory_ptr g_heap_factory;

namespace Core {

Memory_ptr g_core_heap = Memory_ptr::nil ();
Scheduler_ptr g_scheduler;

}
}
