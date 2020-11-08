#ifndef NIRVANA_CORE_COREOBJECT_H_
#define NIRVANA_CORE_COREOBJECT_H_

#include "Heap.h"

namespace Nirvana {
namespace Core {

/// \brief Object allocated from the core heap.
class CoreObject
{
public:
	void* operator new (size_t cb)
	{
		return g_core_heap.allocate (nullptr, cb, 0);
	}

	void operator delete (void* p, size_t cb)
	{
		g_core_heap.release (p, cb);
	}

	void* operator new (size_t cb, void* place)
	{
		return place;
	}

	void operator delete (void*, void*)
	{}
};

}
}

#endif
