// Nirvana project.
// Execution domain (coroutine, fiber).

#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

ObjectPool <ExecDomain> ExecDomain::pool_;

void ExecDomain::spawn (DeadlineTime deadline, SyncDomain* sync_domain)
{
	assert (&ExecContext::current () != this);
	assert (runnable_);
	start ([this, sync_domain]() {this->schedule (sync_domain, false); }, deadline, sync_domain);
}

void ExecDomain::ctor_base ()
{
	wait_list_next_ = nullptr;
	deadline_ = std::numeric_limits <DeadlineTime>::max ();
	sync_domain_ = nullptr;
	ret_qnodes_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
	scheduler_item_created_ = false;
}

inline
void ExecDomain::ret_qnode_push (SyncDomain& sd)
{
	ret_qnodes_ = sd.create_queue_node (ret_qnodes_);
}

inline
SyncDomain::QueueNode* ExecDomain::ret_qnode_pop ()
{
	assert (ret_qnodes_);
	SyncDomain::QueueNode* ret = ret_qnodes_;
	ret_qnodes_ = ret->next ();
	return ret;
}

void ExecDomain::schedule (SyncDomain* sync_domain, bool ret)
{
	assert (&ExecContext::current () != this);

	SyncDomain* old_domain = sync_domain_;
	if (!ret && old_domain)
		ret_qnode_push (*old_domain);
	if (!sync_domain && !scheduler_item_created_) {
		Scheduler::create_item ();
		scheduler_item_created_ = true;
	}

	if ((sync_domain_ = sync_domain)) {
		if (ret)
			return_to_sync_domain ();
		else {
			try {
				sync_domain->schedule (deadline (), *this);
			} catch (...) {
				if (old_domain)
					ret_qnode_pop ()->release ();
				sync_domain_ = old_domain;
				throw;
			}
		}
	} else
		Scheduler::schedule (deadline (), *this);
}

void ExecDomain::return_to_sync_domain ()
{
	SyncDomain::QueueNode* qn = ret_qnode_pop ();
	sync_domain_->schedule (qn, deadline (), *this);
}

void ExecDomain::suspend ()
{
	assert (&ExecContext::current () == &Thread::current ().neutral_context ());
	assert (sync_domain_ || scheduler_item_created_);
	if (sync_domain_)
		ret_qnode_push (*sync_domain_);
}

void ExecDomain::resume ()
{
	assert (&ExecContext::current () != this);
	if (sync_domain_)
		return_to_sync_domain ();
	else {
		assert (scheduler_item_created_);
		Scheduler::schedule (deadline (), *this);
	}
}

void ExecDomain::execute (Word scheduler_error)
{
	Thread::current ().exec_domain (this);
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

void ExecDomain::run ()
{
	assert (&ExecContext::current () != this);
	pool_.release (static_cast <ImplPoolable <ExecDomain>&> (*this));
}

}
}
