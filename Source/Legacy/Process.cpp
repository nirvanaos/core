#include "Process.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

Nirvana::Core::CoreRef <Process> Process::spawn (Nirvana::Core::Runnable& runnable)
{
	auto process = Nirvana::Core::CoreRef <Process>::create <Nirvana::Core::ImplDynamic <Process>> ();
	process->start (runnable, *process);
	return process;
}

}
}
}
