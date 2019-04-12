// Nirvana project.
// Execution domain (fiber).

#include "ExecDomain.h"
#include "Thread.h"
#include <memory>

namespace Nirvana {
namespace Core {

void ExecContext::RunnableHolder::run (::CORBA::Environment& env)
{
	::CORBA::Nirvana::Bridge <Runnable>* bridge = *this;
	(bridge->_epv ().epv.run) (bridge, &env);
}

bool ExecContext::run ()
{
	environment_.clear ();
	if (runnable_) {
		runnable_.run (environment_);
		runnable_ = Runnable::_nil ();
		return true;
	}
	return false;
}

void ExecContext::run_in_neutral_context (Runnable_ptr runnable)
{
	ExecContext* neutral_context = Thread::current ().neutral_context ();
	neutral_context->runnable_ = runnable;
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
