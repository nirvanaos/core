// Nirvana project.
// Execution domain (coroutine, fiber).

#include "SyncDomain.h"
#include "Thread.h"

namespace Nirvana {
namespace Core {

ImplStatic <ExecDomain::Release> ExecDomain::release_;
ObjectPool <ExecDomain> ExecDomain::pool_;

void ExecDomain::Release::run ()
{
	Thread& thread = Thread::current ();
	ExecDomain* ed = thread.exec_domain ();
	thread.exec_domain (nullptr);
	ed->_remove_ref ();
}

void ExecDomain::async_call (Runnable& runnable, DeadlineTime deadline, SyncDomain* sync_domain, CORBA::Nirvana::Interface_ptr environment)
{
	Core_var <ExecDomain> exec_domain = get ();
	ExecDomain* p = exec_domain;
	p->start (runnable, deadline, sync_domain, environment, [p, sync_domain]() {p->schedule (sync_domain, false); });
}

void ExecDomain::schedule (SyncDomain* sync_domain, bool ret)
{
	assert (&ExecContext::current () != this);

	SyncDomain* old_domain = sync_domain_;
	if (!ret && old_domain)
		old_domain->call_begin ();
	try {
		if ((sync_domain_ = sync_domain))
			sync_domain_->schedule (*this, ret);
		else
			Scheduler::schedule (deadline (), *this, 0, ret);
	} catch (...) {
		if (ret)
			unrecoverable_error ();
		else if ((sync_domain_ = old_domain)) {
			try {
				old_domain->schedule (*this, true);
			} catch (...) {
				unrecoverable_error ();
			}
		}
		throw;
	}
}

void ExecDomain::execute (DeadlineTime deadline, Word scheduler_error)
{
	assert (deadline_ == deadline);
	Thread::current ().exec_domain (this);
	scheduler_error_ = scheduler_error;
	ExecContext::switch_to ();
}

void ExecDomain::suspend ()
{
	assert (&ExecContext::current () != this);
	if (sync_domain_)
		sync_domain_->call_begin ();
}

void ExecDomain::execute_loop ()
{
	while (run ()) {
		release ();
	}
}

}
}
