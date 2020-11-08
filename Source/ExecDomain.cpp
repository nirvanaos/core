// Nirvana project.
// Execution domain (coroutine, fiber).

#include "SyncDomain.inl"

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
/*
void ExecDomain::async_call (DeadlineTime deadline, SyncDomain* sync_domain, Runnable& runnable)
{
	Core_var <ExecDomain> exec_domain = get ();
	ExecDomain* p = exec_domain;
	p->start ([p, sync_domain]() {p->schedule (sync_domain, false); }, deadline, sync_domain, runnable);
}
*/
void ExecDomain::spawn (DeadlineTime deadline, SyncDomain* sync_domain)
{
	start ([this, sync_domain]() {this->schedule (sync_domain, false); }, deadline, sync_domain);
}

void ExecDomain::ctor_base ()
{
	wait_list_next_ = nullptr;
	deadline_ = std::numeric_limits <DeadlineTime>::max ();
	sync_domain_ = nullptr;
	ret_qnodes_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
	scheduler_item_ = nullptr;
}

inline
void ExecDomain::ret_qnode_push (SyncDomain& sd)
{
	ret_qnodes_ = sd.queue_node_create ((DeadlineTime)&sd, (Executor*)ret_qnodes_);
}

inline
SyncDomain::QueueNode* ExecDomain::ret_qnode_pop ()
{
	assert (ret_qnodes_);
	SyncDomain::QueueNode* ret = ret_qnodes_;
	ret_qnodes_ = (SyncDomain::QueueNode*)(ret->value ().val);
	return ret;
}

void ExecDomain::schedule (SyncDomain* sync_domain, bool ret)
{
	assert (&ExecContext::current () != this);

	SyncDomain* old_domain = sync_domain_;
	if (!ret && old_domain)
		ret_qnode_push (*old_domain);
	if (!sync_domain && !scheduler_item_)
		scheduler_item_ = Scheduler::create_item (*this);

	if ((sync_domain_ = sync_domain)) {
		if (ret)
			return_to_sync_domain ();
		else {
			try {
				sync_domain->schedule (*this);
			} catch (...) {
				if (old_domain)
					old_domain->queue_node_release (ret_qnode_pop ());
				sync_domain_ = old_domain;
				throw;
			}
		}
	} else
		Scheduler::schedule (deadline (), scheduler_item_);
}

void ExecDomain::return_to_sync_domain ()
{
	SyncDomain::QueueNode* qn = ret_qnode_pop ();
	SyncDomain::QueueNode::Value& val = qn->value ();
	assert (sync_domain_ == (SyncDomain*)(val.deadline));
	val.deadline = deadline ();
	val.val = this;
	sync_domain_->schedule (qn);
	sync_domain_->queue_node_release (qn);
}

void ExecDomain::suspend ()
{
	assert (&ExecContext::current () == &Thread::current ().neutral_context ());
	assert (sync_domain_ || scheduler_item_);
	if (sync_domain_)
		ret_qnode_push (*sync_domain_);
}

void ExecDomain::resume ()
{
	assert (&ExecContext::current () != this);
	if (sync_domain_)
		return_to_sync_domain ();
	else
		Scheduler::schedule (deadline (), scheduler_item_);
}

void ExecDomain::execute (Word scheduler_error)
{
	Thread::current ().exec_domain (this);
	scheduler_error_ = (CORBA::Exception::Code)scheduler_error;
	ExecContext::switch_to ();
}

void ExecDomain::execute_loop ()
{
	while (runnable_) {
		if (scheduler_error_) {
			runnable_->on_crash (scheduler_error_);
			runnable_.reset ();
		} else {
			run ();
		}
		release ();
	}
}

}
}
