// The Nirvana project.
// Core.
// Thread class.

#include "ThreadWorker.h"
#include "Scheduler.h"
#include "Thread.h"

namespace Nirvana {
namespace Core {

/// This static method is called by the scheduler.
void ThreadWorker::execute (Executor& executor, Word scheduler_error) NIRVANA_NOEXCEPT
{
	// Always called in the neutral context.
	assert (&ExecContext::current () == &Core::Thread::current ().neutral_context ());
	// Switch to executor context.
	executor.execute (scheduler_error);
	// Perform possible neutral context calls, then return.
	ExecContext::neutral_context_loop ();
}

void ThreadWorker::yield () NIRVANA_NOEXCEPT
{
	exec_domain (nullptr); // Release worker thread to a pool.
}

}
}
