#include "StartupSys.h"
#include "Scheduler.h"

namespace Nirvana {
namespace Core {

void StartupSys::run ()
{
	Startup::run ();
	// TODO: System domain startup code.
	ret_ = 0;
}

}
}
