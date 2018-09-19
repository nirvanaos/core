// The Nirvana project.
// Core.
// Thread class.

#include "Thread.h"
#include "ExecContext.h"

namespace Nirvana {
namespace Core {

/// This static method is called by the scheduler.
void Thread::execute (Executor_ptr executor, DeadlineTime deadline)
{
	// Always called in the neutral context.
	assert (ExecContext::current () == Thread::current ().neutral_context ());
	// Switch to executor context.
	executor->execute (deadline);
	// Perform possible neutral context calls, then return.
	ExecContext::neutral_context_proc ();
}

}
}
