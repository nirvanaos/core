// The Nirvana project.
// Core.
// Thread class.

#include "Thread.h"
#include "Scheduler.h"
#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

/// This static method is called by the scheduler.
void Thread::execute (Executor& executor, DeadlineTime deadline)
{
	// Always called in the neutral context.
	assert (&ExecContext::current () == &Thread::current ().neutral_context ());
	// Switch to executor context.
	executor.execute (deadline);
	// Perform possible neutral context calls, then return.
	ExecContext::neutral_context_loop ();
	Scheduler::core_free ();
}

SynchronizationContext& Thread::sync_context ()
{
	assert (exec_domain_);
	SyncDomain* sd = exec_domain_->cur_sync_domain ();
	if (sd)
		return *sd;
	else
		return SynchronizationContext::free_sync_context ();
}

RuntimeSupportImpl& Thread::runtime_support ()
{
	assert (exec_domain_);
	SyncDomain* sd = exec_domain_->cur_sync_domain ();
	if (sd)
		return sd->runtime_support ();
	else
		return exec_domain_->runtime_support ();
}

}
}
