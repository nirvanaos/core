// Nirvana project.
// Execution domain (coroutine, fiber).

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

ObjectPoolT <ExecDomain> ExecDomain::pool_;

void ExecDomain::async_call (Runnable_ptr runnable, DeadlineTime deadline, SyncDomain* sync_domain)
{
	ExecDomain* exec_domain = pool_.get ();

	exec_domain->runnable_ = Runnable::_duplicate (runnable);
	exec_domain->deadline_ = deadline;
	exec_domain->cur_sync_domain_ = sync_domain;
	exec_domain->schedule_internal ();
}

void ExecDomain::schedule (SyncDomain* sync_domain)
{
	cur_sync_domain_ = sync_domain;
	if (ExecContext::current () == this)
		run_in_neutral_context (Schedule::_this ());
	else
		schedule_internal ();
}

void ExecDomain::schedule_internal ()
{
	if (cur_sync_domain_) {
		try {
			cur_sync_domain_->schedule (*this);
		} catch (...) {
			cur_sync_domain_ = nullptr;
			throw;
		}
	} else
		scheduler ()->schedule (deadline (), this, 0);
}

void ExecDomain::execute_loop ()
{
	while (run ()) {
		environment_.clear ();	// TODO: Check unhandled exception and log error message.
		run_in_neutral_context (Release::_this ());
	}
}

}
}
