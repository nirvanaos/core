// Nirvana project.
// Execution domain (coroutine, fiber).

#include "Thread.inl"

namespace Nirvana {
namespace Core {

ObjectPool <ExecDomain> ExecDomain::pool_;

void ExecDomain::ReleaseToPool::run ()
{
	assert (&ExecContext::current () != &obj_);
	pool_.release (static_cast <ImplPoolable <ExecDomain>&> (obj_));
}

void ExecDomain::ctor_base ()
{
	Scheduler::activity_begin ();
	release_to_pool_ = Core_var <Runnable>::create <ImplDynamic <ReleaseToPool> > (std::ref (*this));
	wait_list_next_ = nullptr;
	deadline_ = std::numeric_limits <DeadlineTime>::max ();
	sync_context_ = nullptr;
	ret_qnodes_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
	scheduler_item_created_ = false;
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

	SyncContext* old_context = sync_context_;
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
		sync_context_ = old_context;
		throw;
	}
}

void ExecDomain::suspend ()
{
	ExecDomain* cur = Thread::current ().exec_domain ();
	assert (cur);
	cur->sync_context ()->schedule_call (SyncContext::SUSPEND);
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

void ExecDomain::execute_loop ()
{
	assert (Thread::current ().exec_domain () == this);
	while (runnable_) {
		if (scheduler_error_) {
			runnable_->on_crash (scheduler_error_);
			runnable_.reset ();
		} else {
			ExecContext::run ();
		}
		Thread::current ().exec_domain (nullptr);
		_remove_ref ();
	}
}

}
}
