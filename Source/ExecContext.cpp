// Nirvana project.
// Execution context (fiber).

#include "ExecDomain.h"
#include "Thread.h"
#include <memory>

namespace Nirvana {
namespace Core {

void ExecContext::run () NIRVANA_NOEXCEPT
{
	assert (runnable_);
	try {
		runnable_->run ();
	} catch (...) {
		runnable_->on_exception ();
	}
	runnable_ = nullptr;
}

void ExecContext::on_crash (CORBA::SystemException::Code err) NIRVANA_NOEXCEPT
{
	runnable_->on_crash (err);
	runnable_ = nullptr;
}

void ExecContext::neutral_context_loop () NIRVANA_NOEXCEPT
{
	Thread& thread = Thread::current ();
	assert (&current () == &thread.neutral_context ());
	for (;;) {
		thread.neutral_context ().run ();
		if (thread.exec_domain ())
			thread.exec_domain ()->switch_to ();
		else
			break;
	}
}

}
}
