#ifndef NIRVANA_LEGACY_CORE_PROCESS_H_
#define NIRVANA_LEGACY_CORE_PROCESS_H_

#include "ThreadBackground.h"
#include "CoreObject.h"
#include "RuntimeSupportLegacy.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

class Process :
	public ThreadBackground,
	public Nirvana::Core::CoreObject
{
public:
	static void spawn (int argc, char* argv []);
	static Nirvana::Core::Core_var <Process> posix_spawn (const char* file, char* argv [], char* envp);

private:
	Nirvana::Core::Heap heap_;
	RuntimeSupportLegacy runtime_support_;
};

}
}
}

#endif
