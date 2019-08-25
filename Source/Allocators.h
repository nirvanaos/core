#ifndef NIRVANA_CORE_ALLOCATORS_H_
#define NIRVANA_CORE_ALLOCATORS_H_

#include <memory>
#include "Stack.h"

namespace Nirvana {
namespace Core {

template <class T>
class CoreAllocator :
	public std::allocator <T>
{
public:
	void deallocate (T* p, size_t cnt)
	{
		g_core_heap->release (p, cnt * sizeof (T));
	}

	T* allocate (size_t cnt, void* hint = nullptr)
	{
		return (T*)g_core_heap->allocate (0, cnt * sizeof (T), 0);
	}
};

template <unsigned SIZE>
class FreeList :
	private Stack <SIZE>
{
public:
	void push (void* p)
	{
		Stack <SIZE>::push ((StackElem*)p);
	}

	void* pop ()
	{
		return Stack <SIZE>::pop ();
	}
};

template <class T>
class AllocatorFixedSize :
	public std::allocator <T>
{
public:
	void deallocate (T* p, size_t cnt)
	{
		assert (1 == cnt);
		free_list_.push (p);
	}

	T* allocate (size_t cnt, void* hint = nullptr)
	{
		assert (1 == cnt);
		T* p = (T*)free_list_.pop ();
		if (!p)
			p = (T*)g_core_heap->allocate (0, sizeof (T), 0);
		return p;
	}

private:
	static FreeList <std::max ((unsigned)HEAP_UNIT_CORE, (unsigned)(1 << log2_ceil (sizeof (T))))> free_list_;
};

}
}

#endif
