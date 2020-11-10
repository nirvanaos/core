// Nirvana project.
// Execution domain (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "SyncDomain.h"
#include "Scheduler.h"
#include "ObjectPool.h"
#include "CoreObject.h"
#include "RuntimeSupportImpl.h"
#include <limits>
#include <utility>

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE ExecDomain :
	public CoreObject,
	public ExecContext,
	public Executor,
	private Runnable // Runnable is used for return object to pool in neutral context.
{
	static const size_t MAX_RUNNABLE_SIZE = 8; // In words.
public:
	template <class R, class ... Args>
	static void async_call (DeadlineTime deadline, SyncDomain* sync_domain, Args ... args)
	{
		Core_var <ExecDomain> exec_domain = create <R> (std::forward <Args> (args)...);
		exec_domain->spawn (deadline, sync_domain);
	}

	static void async_call (DeadlineTime deadline, Runnable& runnable, SyncDomain* sync_domain)
	{
		Core_var <ExecDomain> exec_domain = create (runnable);
		exec_domain->spawn (deadline, sync_domain);
	}

	static Core_var <ExecDomain> create (Runnable& runnable)
	{
		Core_var <ExecDomain> exec_domain = get ();
		exec_domain->runnable_ = &runnable;
		return exec_domain;
	}

	template <class R, class ... Args>
	static Core_var <ExecDomain> create (Args ... args)
	{
		Core_var <ExecDomain> exec_domain = get ();
		static_assert (sizeof (ImplNoAddRef <R>) <= sizeof (runnable_space_), "Runnable object is too large.");
		exec_domain->runnable_ = Core_var <Runnable>::construct <ImplNoAddRef <R> > (exec_domain->runnable_space_, std::forward <Args> (args)...);
		return exec_domain;
	}

	/// Used in porting for creation of the startup domain.
	template <class ... Args>
	static Core_var <ExecDomain> create_main (Runnable& startup, Args ... args)
	{
		return Core_var <ExecDomain>::create <ImplPoolable <ExecDomain> > (std::ref (pool_), std::ref (startup), std::forward <Args> (args)...);
	}

	DeadlineTime deadline () const
	{
		return deadline_;
	}

	/// Schedules this ED to execute.
	/// Must be called from another execution context.
	/// Does not throw an exception if `ret = true`.
	///
	/// \param sync_domain Synchronization domain. May be `nullptr`.
	/// \param ret `false` on call, `true` on return.
	void schedule (SyncDomain* sync_domain, bool ret);

	/// Executor::execute ()
	/// Called from worker thread.
	void execute (Word scheduler_error);

	template <class Starter>
	void start (Starter starter, DeadlineTime deadline, SyncDomain* sync_domain)
	{
		assert (runnable_);
		deadline_ = deadline;
		sync_domain_ = sync_domain;
		starter ();
		_add_ref ();
	}

	void spawn (DeadlineTime deadline, SyncDomain* sync_domain);

	void suspend ();
	void resume ();

	void _activate ()
	{}

	void _deactivate (ImplPoolable <ExecDomain>& obj)
	{
		runnable_.reset ();
		sync_domain_ = nullptr;
		scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
		runtime_support_.cleanup ();
		heap_.cleanup ();
		ret_qnodes_clear ();
		if (scheduler_item_) {
			Scheduler::release_item (scheduler_item_);
			scheduler_item_ = 0;
		}
		Thread& thread = Thread::current ();
		if (thread.exec_domain () == this)
			thread.exec_domain (nullptr);
		Scheduler::activity_end ();
		if (&ExecContext::current () == this)
			run_in_neutral_context (*this);
		else
			pool_.release (obj);
	}

	void execute_loop ();

	void on_crash (Word code)
	{
		ExecContext::on_crash (code);
		_remove_ref ();
	}

	SyncDomain* sync_domain () const
	{
		return sync_domain_;
	}

	Heap& heap ()
	{
		return heap_;
	}

	RuntimeSupportImpl& runtime_support ()
	{
		return runtime_support_;
	}

	CORBA::Exception::Code scheduler_error () const
	{
		return scheduler_error_;
	}

protected:
	ExecDomain () :
		ExecContext ()
	{
		ctor_base ();
	}

	//! Constructor with parameters used in porting for creation of the startup domain.
	template <class ... Args>
	ExecDomain (Runnable& startup, Args ... args) :
		ExecContext (std::forward <Args> (args)...)
	{
		ctor_base ();
		runnable_ = &startup;
		Scheduler::activity_begin ();
	}

	friend class Core_var <ExecDomain>;
	virtual void _add_ref () = 0;
	virtual void _remove_ref () = 0;

private:
	void ctor_base ();

	void ret_qnode_push (SyncDomain& sd);
	SyncDomain::QueueNode* ret_qnode_pop ();

	void ret_qnodes_clear ()
	{
		while (ret_qnodes_) {
			SyncDomain::QueueNode* qn = ret_qnodes_;
			ret_qnodes_ = (SyncDomain::QueueNode*)(qn->value ().val);
			((SyncDomain*)(qn->value ().deadline))->queue_node_release (qn);
		}
	}

	static Core_var <ExecDomain> get ()
	{
		Scheduler::activity_begin ();	// Throws exception if shutdown was started.
		return pool_.get ();
	}

	void return_to_sync_domain ();

	virtual void run ();

public:
	ExecDomain* wait_list_next_;

private:
	static ObjectPool <ExecDomain> pool_;

	DeadlineTime deadline_;
	SyncDomain* sync_domain_;
	SyncDomain::QueueNode* ret_qnodes_;
	Heap heap_;
	RuntimeSupportImpl runtime_support_;
	Scheduler::Item scheduler_item_;
	CORBA::Exception::Code scheduler_error_;

	Word runnable_space_ [MAX_RUNNABLE_SIZE];
};

}
}

#endif
