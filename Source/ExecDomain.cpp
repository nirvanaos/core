// Nirvana project.
// Execution domain (coroutine, fiber).

#include "ExecDomain.h"

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

void ExecDomain::ctor_base ()
{
	wait_list_next_ = nullptr;
	deadline_ = std::numeric_limits <DeadlineTime>::max ();
	sync_domain_ = nullptr;
	ret_qnodes_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
}

inline
void ExecDomain::ret_qnode_push (SyncDomain& sd)
{
	ret_qnodes_ = sd.queue_node_create ((DeadlineTime)&sd, (ExecDomain*)ret_qnodes_);
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
	try {
		if ((sync_domain_ = sync_domain)) {
			if (ret) {
				SyncDomain::QueueNode* qn = ret_qnode_pop ();
				PriorityQueueKeyVal <ExecDomain*>& val = qn->value ();
				assert (sync_domain == (SyncDomain*)(val.deadline));
				val.deadline = deadline ();
				val.val = this;
				sync_domain->schedule (qn);
				sync_domain->queue_node_release (qn);
			} else
				sync_domain->schedule (*this);
		} else
			Scheduler::schedule (deadline (), *this, 0, ret);
	} catch (...) {
		if (!ret && old_domain)
			old_domain->queue_node_release (ret_qnode_pop ());
		sync_domain_ = old_domain;
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
		ret_qnode_push(*sync_domain_);
}

void ExecDomain::resume ()
{
	assert (&ExecContext::current () != this);
	if (sync_domain_) {
		SyncDomain::QueueNode* qn = ret_qnode_pop ();
		PriorityQueueKeyVal <ExecDomain*>& val = qn->value ();
		assert (sync_domain_ == (SyncDomain*)(val.deadline));
		val.deadline = deadline ();
		val.val = this;
		sync_domain_->schedule (qn);
		sync_domain_->queue_node_release (qn);
	} else
		Scheduler::schedule (deadline (), *this, 0, true);
}

void ExecDomain::execute_loop ()
{
	if (scheduler_error_) {
		CORBA::Nirvana::set_exception (environment_, scheduler_error_, nullptr, nullptr);
		environment_ = CORBA::Nirvana::Interface::_nil ();
		runnable_.reset ();
	} else {
		while (run ()) {
			release ();
		}
	}
}

}
}
