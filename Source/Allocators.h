#ifndef NIRVANA_CORE_ALLOCATORS_H_
#define NIRVANA_CORE_ALLOCATORS_H_

#include "core.h"
#include "Stack.h"

namespace Nirvana {
namespace Core {

template <unsigned SIZE>
class FreeList :
	private Stack <StackElem, SIZE>
{
public:
	void push (void* p)
	{
		Stack <StackElem, SIZE>::push (*(StackElem*)p);
	}

	void* pop ()
	{
		return Stack <StackElem, SIZE>::pop ();
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
	static FreeList <CORE_OBJECT_ALIGN (T)> free_list_;
};

}
}

#endif
