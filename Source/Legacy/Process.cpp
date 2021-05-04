#include "Process.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

Nirvana::Core::Core_var <Process> Process::spawn (Nirvana::Core::Runnable& runnable)
{
	auto var = Nirvana::Core::Core_var <Process>::create <Nirvana::Core::ImplDynamic <Process> > ();
	var->start (var->runtime_support_, runnable);
	return var;
}

Nirvana::Core::Heap& Process::memory () NIRVANA_NOEXCEPT
{
	return heap_;
}

}
}
}
