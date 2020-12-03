// Nirvana project.
// Execution domain (coroutine, fiber).

#include "Thread.inl"

namespace Nirvana {
namespace Core {

ObjectPool <ExecDomain> ExecDomain::pool_;

void ExecDomain::run ()
{
	assert (&ExecContext::current () != this);
	pool_.release (static_cast <ImplPoolable <ExecDomain>&> (*this));
}

void ExecDomain::ctor_base ()
{
	Scheduler::activity_begin ();
	wait_list_next_ = nullptr;
	deadline_ = std::numeric_limits <DeadlineTime>::max ();
	ret_qnodes_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
	scheduler_item_created_ = false;
	stateless_creation_frame_ = nullptr;
}

void ExecDomain::spawn (SyncDomain* sync_domain)
{
	assert (&ExecContext::current () != this);
	assert (runnable_);
	start ([this, sync_domain]() {this->schedule (sync_domain); });
}

void ExecDomain::schedule (SyncDomain* sync_domain)
{
	assert (&ExecContext::current () != this);

	Core_var <SyncContext> old_context = std::move (sync_context_);
	if (!sync_domain && !scheduler_item_created_) {
		Scheduler::create_item ();
		scheduler_item_created_ = true;
	}

	try {
		if (sync_domain) {
			sync_context_ = sync_domain;
			sync_domain->schedule (deadline (), *this);
		} else {
			sync_context_ = &SyncContext::free_sync_context ();
			Scheduler::schedule (deadline (), *this);
		}
	} catch (...) {
		sync_context_ = std::move (old_context);
		throw;
	}

	if (old_context) {
		SyncDomain* old_sd = old_context->sync_domain ();
		if (old_sd)
			old_sd->leave ();
	}
}

void ExecDomain::suspend ()
{
	ExecDomain* cur = Thread::current ().exec_domain ();
	assert (cur);
	cur->sync_context ()->schedule_call (SyncContext::SUSPEND ());
}

void ExecDomain::resume ()
{
	assert (&ExecContext::current () != this);
	assert (sync_context_);
	sync_context_->schedule_return (*this);
}

void ExecDomain::execute (Word scheduler_error)
{
	Thread::current ().exec_domain (*this);
	scheduler_error_ = (CORBA::Exception::Code)scheduler_error;
	ExecContext::switch_to ();
}

void ExecDomain::cleanup () NIRVANA_NOEXCEPT
{
	{
		SyncDomain* sd = sync_context_->sync_domain ();
		if (sd)
			sd->leave ();
	}
	runtime_support_.cleanup ();
	heap_.cleanup (); // TODO: Detect and log the memory leaks.
	sync_context_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
	ret_qnodes_clear ();
	if (scheduler_item_created_) {
		Scheduler::delete_item ();
		scheduler_item_created_ = false;
	}
}

void ExecDomain::execute_loop () NIRVANA_NOEXCEPT
{
	assert (Thread::current ().exec_domain () == this);
	while (runnable_) {
		if (scheduler_error_) {
			runnable_->on_crash (scheduler_error_);
			runnable_ = nullptr;
		} else {
			ExecContext::run ();
			assert (!runnable_); // Cleaned inside ExecContext::run ();
		}
		cleanup ();
		Thread::current ().exec_domain (nullptr);
		_remove_ref ();
	}
}

void ExecDomain::on_exec_domain_crash (CORBA::SystemException::Code err) NIRVANA_NOEXCEPT
{
	Thread::current ().exec_domain (nullptr);
	ExecContext::on_crash (err);
	cleanup ();
	_remove_ref ();
}

}
}
