#include "user_memory.h"
#include "SyncContext.h"

namespace Nirvana {
namespace Core {

Heap& user_memory ()
{
	return SyncContext::current ().memory ();
}

}
}
