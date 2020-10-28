#ifndef NIRVANA_CORE_USEROBJECT_H_
#define NIRVANA_CORE_USEROBJECT_H_

#include "user_memory.h"

namespace Nirvana {
namespace Core {

/// \brief Object allocated from the user heap.
class UserObject
{
public:
	void* operator new (size_t cb)
	{
		return user_memory ().allocate (nullptr, cb, 0);
	}

	void operator delete (void* p, size_t cb)
	{
		user_memory ().release (p, cb);
	}
};

}
}

#endif
