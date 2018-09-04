#ifndef NIRVANA_CORE_THREADWORKER_H_
#define NIRVANA_CORE_THREADWORKER_H_

#include "Thread.h"
#include "SyncDomain.h"

#ifdef _WIN32
#include "Windows/ThreadWorkerBase.h"
namespace Nirvana {
namespace Core {
typedef Windows::ThreadWorkerBase ThreadWorkerBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class ThreadWorker :
	public ThreadWorkerBase
{
public:
	/// Perform earlier deadline task from the specified synchronization domain's queue.
	/// This function is called by the system scheduler.
	void execute (SyncDomain& sync_domain)
	{
		sync_domain.execute ();
		neutral_context_proc ();
	}
};

}
}

#endif
