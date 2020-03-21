#include "Heap.h"
#include <Nirvana/HeapFactory_s.h>
#include <Nirvana/OLF.h>

namespace Nirvana {
namespace Core {

class HeapDynamic :
	public Heap,
	public CORBA::Nirvana::Servant <HeapDynamic, Memory>,
	public CORBA::Nirvana::LifeCycleRefCntImpl <HeapDynamic>
{
public:
	HeapDynamic (ULong allocation_unit) :
		Heap (allocation_unit)
	{}
};

class HeapFactoryImpl :
	public ::CORBA::Nirvana::ServantStatic <HeapFactoryImpl, HeapFactory>
{
public:
	static Memory_ptr create ()
	{
		return (new HeapDynamic (HEAP_UNIT_DEFAULT))->_get_ptr ();
	}
	static Memory_ptr create_with_granularity (ULong granularity)
	{
		return (new HeapDynamic (granularity))->_get_ptr ();
	}
};



}
}
