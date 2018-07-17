// Nirvana project.
// Core implementation definitions

#ifndef NIRVANA_CORE_CORE_H_
#define NIRVANA_CORE_CORE_H_

#include <Nirvana.h>
#include <HeapFactory.h>

namespace Nirvana {

void initialize ();
void terminate ();

extern Memory_ptr g_protection_domain_memory;
extern Memory_ptr g_default_heap;
extern HeapFactory_ptr g_heap_factory;

}

#endif
