// Nirvana project.
// Execution context (fiber).

#include "ExecDomain.h"
#include "Thread.h"
#include <memory>

namespace Nirvana {
namespace Core {

ExecContext& ExecContext::current ()
{
	return Thread::current ().context ();
}

void ExecContext::switch_to ()
{
	assert (&current () != this);
	Thread::current ().context (*this);
	Port::ExecContext::switch_to ();
}

bool ExecContext::run ()
{
	if (runnable_) {
		try {
			runnable_->run ();
		} catch (...) {
			runnable_->on_exception ();
		}
		runnable_.reset ();
		return true;
	}
	return false;
}

void ExecContext::on_crash (Word code)
{
	runnable_->on_crash (code);
	runnable_.reset ();
}

void ExecContext::neutral_context_loop ()
{
	Thread& thread = Thread::current ();
	ExecContext& context = thread.context ();
	assert (&context == &thread.neutral_context ());
	for (;;) {
		context.run ();
		if (thread.exec_domain ())
			thread.exec_domain ()->switch_to ();
		else
			break;
	}
}

}
}
