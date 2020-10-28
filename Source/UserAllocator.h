#ifndef NIRVANA_CORE_USERALLOCATOR_H_
#define NIRVANA_CORE_USERALLOCATOR_H_

#include "user_memory.h"

namespace Nirvana {
namespace Core {

template <class T>
class UserAllocator :
	public std::allocator <T>
{
public:
	static void deallocate (T* p, size_t cnt)
	{
		user_memory ().release (p, cnt * sizeof (T));
	}

	static T* allocate (size_t cnt, void* hint = nullptr, UWord flags = 0)
	{
		return (T*)user_memory ().allocate (hint, cnt * sizeof (T), flags);
	}
};

}
}

#endif
