// The Nirvana project.
// Core.
// Thread class.

#include "ThreadWorker.h"
#include "Scheduler.h"
#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

/// This static method is called by the scheduler.
void ThreadWorker::execute (Executor& executor, DeadlineTime deadline, Word scheduler_error)
{
	// Always called in the neutral context.
	assert (&ExecContext::current () == &Core::Thread::current ().neutral_context ());
	// Switch to executor context.
	executor.execute (deadline, scheduler_error);
	// Perform possible neutral context calls, then return.
	ExecContext::neutral_context_loop ();
	Scheduler::core_free ();
}

SyncContext& ThreadWorker::sync_context () NIRVANA_NOEXCEPT
{
	assert (exec_domain ());
	SyncDomain* sd = exec_domain ()->sync_domain ();
	if (sd)
		return *sd;
	else
		return SyncContext::free_sync_context ();
}

RuntimeSupportImpl& ThreadWorker::runtime_support () NIRVANA_NOEXCEPT
{
	assert (exec_domain ());
	SyncDomain* sd = exec_domain ()->sync_domain ();
	if (sd)
		return sd->runtime_support ();
	else
		return exec_domain ()->runtime_support ();
}

void ThreadWorker::run ()
{
	Core::Thread::run ();
	exec_domain (nullptr); // Release worker thread to a pool.
}

}
}
