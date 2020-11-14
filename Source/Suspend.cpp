#include "Suspend.h"
#include "Thread.h"

namespace Nirvana {
namespace Core {

void Suspend::run ()
{
	Thread::current ().yield ();
}

}
}
