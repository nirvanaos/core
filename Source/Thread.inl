#ifndef NIRVANA_CORE_THREAD_INL_
#define NIRVANA_CORE_THREAD_INL_

#include "Thread.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

inline
void Thread::exec_domain (ExecDomain& exec_domain) NIRVANA_NOEXCEPT
{
	exec_domain_ = &exec_domain;
	SyncDomain* sd = exec_domain.sync_context ()->sync_domain ();
	if (sd)
		runtime_support_ = &sd->runtime_support ();
	else
		runtime_support_ = &exec_domain.runtime_support ();
}


}
}

#endif
