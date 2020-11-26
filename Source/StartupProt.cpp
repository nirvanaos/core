#include "StartupProt.h"

namespace Nirvana {
namespace Core {

void StartupProt::run ()
{
	Startup::run ();
	// TODO: Protection domain startup code.
	ret_ = 0;
}

}
}
