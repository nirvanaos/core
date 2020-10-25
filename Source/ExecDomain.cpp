// Nirvana project.
// Execution domain (coroutine, fiber).

#include "SyncDomain.h"
#include "Thread.h"

namespace Nirvana {
namespace Core {

ImplStatic <ExecDomain::Release> ExecDomain::release_;
ImplStatic <ExecDomain::Schedule> ExecDomain::schedule_;

void ExecDomain::Release::run ()
{
	Thread::current ().execution_domain (nullptr);
}

void ExecDomain::Schedule::run ()
{
	Thread& thread = Thread::current ();
	thread.execution_domain ()->schedule_internal ();
	thread.execution_domain (nullptr);
}

void ExecDomain::async_call (Runnable& runnable, DeadlineTime deadline, SyncDomain* sync_domain, CORBA::Nirvana::EnvironmentBridge* environment)
{
	Scheduler::activity_begin ();	// Throws exception if shutdown was started.

	Core_var <ExecDomain> exec_domain = get ();

	exec_domain->runnable_ = &runnable;
	exec_domain->environment_ = environment;
	exec_domain->deadline_ = deadline;
	exec_domain->cur_sync_domain_ = sync_domain;
	exec_domain->schedule_internal ();
}

void ExecDomain::schedule (SyncDomain* sync_domain)
{
	if (cur_sync_domain_)
		cur_sync_domain_->leave ();
	cur_sync_domain_ = sync_domain;
	if (ExecContext::current () == this)
		run_in_neutral_context (schedule_);
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
		Scheduler::schedule (deadline (), *this, 0);
}

void ExecDomain::execute (DeadlineTime deadline)
{
	assert (deadline_ == deadline);
	Thread::current ().execution_domain (this);
	ExecContext::switch_to ();
}

void ExecDomain::suspend ()
{
	if (cur_sync_domain_)
		cur_sync_domain_->leave ();
}

void ExecDomain::execute_loop ()
{
	while (run ()) {
		run_in_neutral_context (release_);
	}
}

}
}
