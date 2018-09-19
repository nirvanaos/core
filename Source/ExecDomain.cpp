// Nirvana project.
// Execution domain (coroutine, fiber).

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

ObjectPoolT <ExecDomain> ExecDomain::pool_;

void ExecDomain::async_call (Runnable_ptr runnable, DeadlineTime deadline, SyncDomain* sync_domain)
{
	ExecDomain* exec_domain = pool_.get ();

	exec_domain->runnable_ = runnable;
	exec_domain->deadline_ = deadline;

	if (sync_domain) {
		exec_domain->cur_sync_domain_ = sync_domain;
		sync_domain->schedule (*exec_domain);
	} else
		g_scheduler->schedule (deadline, exec_domain->_this (), 0);
}

}
}
