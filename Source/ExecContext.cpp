// Nirvana project.
// Execution context (fiber).

#include "ExecDomain.h"
#include "Thread.h"
#include <memory>

namespace Nirvana {
namespace Core {

ExecContext* ExecContext::current ()
{
	return Thread::current ().context ();
}

void ExecContext::switch_to ()
{
	assert (current () != this);
	Thread::current ().context (this);
	Port::ExecContext::switch_to ();
}

bool ExecContext::run ()
{
	environment_.exception_free ();
	if (runnable_) {
		(runnable_->_epv ().epv.run) (runnable_, &environment_);
		runnable_ = Runnable::_nil ();
		return true;
	}
	return false;
}

void ExecContext::run_in_neutral_context (Runnable_ptr runnable)
{
	ExecContext* neutral_context = Thread::current ().neutral_context ();
	assert (neutral_context);
	neutral_context->runnable_ = Runnable::_duplicate (runnable);
	neutral_context->switch_to ();
	::std::auto_ptr < ::CORBA::Exception> exception (neutral_context->environment ().detach ());
	if (exception.get ())
		exception->_raise ();
}

void ExecContext::neutral_context_loop ()
{
	Thread& thread = Thread::current ();
	ExecContext* context = current ();
	assert (context == thread.neutral_context ());
	for (;;) {
		context->run ();
		if (thread.execution_domain ())
			thread.execution_domain ()->switch_to ();
		else
			break;
	}
}

}
}
