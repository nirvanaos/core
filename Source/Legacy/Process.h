#ifndef NIRVANA_LEGACY_CORE_PROCESS_H_
#define NIRVANA_LEGACY_CORE_PROCESS_H_

#include "ThreadBackground.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

class Process : public ThreadBackground
{
public:
	static void spawn (int argc, char* argv []);
	static Core_var <Process> posix_spawn (const char* file, char* argv [], char* envp);

private:
	Core::Heap heap_;
	Core::RuntimeSupport runtime_support_;
};

}
}
}

#endif
