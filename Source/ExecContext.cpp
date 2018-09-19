// Nirvana project.
// Execution domain (fiber).

#include "ExecDomain.h"
#include "Thread.h"

namespace Nirvana {
namespace Core {

void ExecContext::run_in_neutral_context (Runnable_ptr runnable)
{
	ExecContext* neutral_context = Thread::current ().neutral_context ();
	neutral_context->runnable_ = runnable;
	neutral_context->switch_to ();
}

void ExecContext::neutral_context_proc ()
{
	Thread& thread = Thread::current ();
	ExecContext* context = current ();
	assert (context == thread.neutral_context ());
	for (;;) {
		Runnable_ptr r = context->runnable_;
		if (r) {
			r->run ();
			context->runnable_ = Runnable::_nil ();
		}
		if (thread.execution_domain ())
			thread.execution_domain ()->switch_to ();
		else
			break;
	}
}

}
}
