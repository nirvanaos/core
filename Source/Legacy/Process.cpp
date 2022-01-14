#include "Process.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

Nirvana::Core::CoreRef <Process> Process::spawn (Nirvana::Core::Runnable& runnable)
{
	auto var = Nirvana::Core::CoreRef <Process>::create <Nirvana::Core::ImplDynamic <Process>> ();
	var->start (runnable, var->mem_context_);
	return var;
}

}
}
}
