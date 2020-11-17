#include "StartupProt.h"
#include "Scheduler.h"

namespace Nirvana {
namespace Core {

void StartupProt::run ()
{
	// TODO: Protection domain startup code.
	Scheduler::shutdown ();
}

}
}
