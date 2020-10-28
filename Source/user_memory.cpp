#include "user_memory.h"
#include "SynchronizationContext.h"

namespace Nirvana {
namespace Core {

Heap& user_memory ()
{
	return SynchronizationContext::current ().memory ();
}

}
}
