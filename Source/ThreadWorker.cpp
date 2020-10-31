// The Nirvana project.
// Core.
// Thread class.

#include "ThreadWorker.h"
#include "Scheduler.h"
#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

/// This static method is called by the scheduler.
void ThreadWorker::execute (Executor& executor, DeadlineTime deadline)
{
	// Always called in the neutral context.
	assert (&ExecContext::current () == &Thread::current ().neutral_context ());
	// Switch to executor context.
	executor.execute (deadline);
	// Perform possible neutral context calls, then return.
	ExecContext::neutral_context_loop ();
	Scheduler::core_free ();
}

SyncContext& ThreadWorker::sync_context ()
{
	assert (execution_domain ());
	SyncDomain* sd = execution_domain ()->cur_sync_domain ();
	if (sd)
		return *sd;
	else
		return SyncContext::free_sync_context ();
}

RuntimeSupportImpl& ThreadWorker::runtime_support ()
{
	assert (execution_domain ());
	SyncDomain* sd = execution_domain ()->cur_sync_domain ();
	if (sd)
		return sd->runtime_support ();
	else
		return execution_domain ()->runtime_support ();
}

}
}
