#include "UserObject.h"
#include "SynchronizationContext.h"

namespace Nirvana {
namespace Core {

void* UserObject::operator new (size_t cb)
{
	return Thread::current ().synchronization_context ()->memory ().allocate (nullptr, cb, 0);
}

void UserObject::operator delete (void* p, size_t cb)
{
	Thread::current ().synchronization_context ()->memory ().release (p, cb);
}

}
}
