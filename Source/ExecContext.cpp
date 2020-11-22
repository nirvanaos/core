// Nirvana project.
// Execution context (fiber).

#include "ExecDomain.h"
#include "Thread.h"
#include <memory>

namespace Nirvana {
namespace Core {

void ExecContext::run () NIRVANA_NOEXCEPT
{
	if (runnable_) {
		try {
			runnable_->run ();
		} catch (...) {
			runnable_->on_exception ();
		}
		runnable_.reset ();
	}
}

void ExecContext::on_crash (CORBA::SystemException::Code err) NIRVANA_NOEXCEPT
{
	runnable_->on_crash (err);
	runnable_.reset ();
}

void ExecContext::neutral_context_loop () NIRVANA_NOEXCEPT
{
	Thread& thread = Thread::current ();
	ExecContext& context = current ();
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
