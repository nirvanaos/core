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
	public Executor
{
	static const size_t MAX_RUNNABLE_SIZE = 8; // In pointers.
public:
	template <class R, class ... Args>
	static void async_call (const DeadlineTime& deadline, SyncDomain* sync_domain, Args ... args)
	{
		static_assert (sizeof (ImplNoAddRef <R>) <= sizeof (runnable_space_), "Runnable object is too large.");
		Core_var <ExecDomain> exec_domain = get (deadline);
		exec_domain->runnable_ = Core_var <Runnable>::construct <ImplNoAddRef <R> > (exec_domain->runnable_space_, std::forward <Args> (args)...);
		exec_domain->spawn (sync_domain);
	}

	static void async_call (const DeadlineTime& deadline, Runnable& runnable, SyncDomain* sync_domain)
	{
		Core_var <ExecDomain> exec_domain = get (deadline);
		exec_domain->runnable_ = &runnable;
		exec_domain->spawn (sync_domain);
	}

	/// Used in porting for creation of the startup domain.
	template <class ... Args>
	static Core_var <ExecDomain> create_main (const DeadlineTime& deadline, Runnable& startup, Args ... args)
	{
		return Core_var <ExecDomain>::create <ImplPoolable <ExecDomain> > (deadline, std::ref (startup), std::forward <Args> (args)...);
	}

	/// Get execution domain for a background thread.
	template <class R, class ... Args>
	static Core_var <ExecDomain> get_background (SyncContext& sync_context, Args ... args)
	{
		static_assert (sizeof (ImplNoAddRef <R>) <= sizeof (runnable_space_), "Runnable object is too large.");
		Core_var <ExecDomain> exec_domain = get (INFINITE_DEADLINE);
		exec_domain->runnable_ = Core_var <Runnable>::construct <ImplNoAddRef <R> > (exec_domain->runnable_space_, std::forward <Args> (args)...);
		exec_domain->sync_context_ = &sync_context;
		return exec_domain;
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
	void schedule (SyncDomain* sync_domain);

	/// Executor::execute ()
	/// Called from worker thread.
	void execute (Word scheduler_error);

	template <class Starter>
	void start (Starter starter)
	{
		assert (runnable_);
		starter ();
		_add_ref ();
	}

	void spawn (SyncDomain* sync_domain);

	/// Suspend current domain
	static void suspend ();

	/// Resume suspended domain
	void resume ();

	void _activate ()
	{}

	/// Clear data and return object to the pool
	void _deactivate (ImplPoolable <ExecDomain>& obj)
	{
		runnable_.reset ();
		sync_context_ = nullptr;
		scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
		runtime_support_.cleanup ();
		heap_.cleanup ();
		ret_qnodes_clear ();
		if (scheduler_item_created_) {
			Scheduler::delete_item ();
			scheduler_item_created_ = false;
		}
		Thread& thread = Thread::current ();
		if (thread.exec_domain () == this)
			thread.exec_domain (nullptr);
		Scheduler::activity_end ();
		if (&ExecContext::current () == this)
			run_in_neutral_context (release_to_pool_);
		else
			pool_.release (obj);
	}

	void execute_loop ();

	void on_exec_domain_crash (CORBA::SystemException::Code err)
	{
		ExecContext::on_crash (err);
		_remove_ref ();
	}

	SyncContext* sync_context () const
	{
		return sync_context_;
	}

	void sync_context (SyncContext& sync_context)
	{
		sync_context_ = &sync_context;
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

	void ret_qnode_push (SyncDomain& sd)
	{
		ret_qnodes_ = sd.create_queue_node (ret_qnodes_);
	}

	SyncDomain::QueueNode* ret_qnode_pop () NIRVANA_NOEXCEPT
	{
		assert (ret_qnodes_);
		SyncDomain::QueueNode* ret = ret_qnodes_;
		ret_qnodes_ = ret->next ();
		return ret;
	}

protected:
	ExecDomain () :
		ExecContext ()
	{
		ctor_base ();
	}

	//! Constructor with parameters used in porting for creation of the startup domain.
	template <class ... Args>
	ExecDomain (DeadlineTime deadline, Runnable& startup, Args ... args) :
		ExecContext (std::forward <Args> (args)...)
	{
		ctor_base ();
		deadline_ = deadline;
		runnable_ = &startup;
	}

	friend class Core_var <ExecDomain>;
	virtual void _add_ref () = 0;
	virtual void _remove_ref () = 0;

private:
	void ctor_base ();

	void ret_qnodes_clear ()
	{
		while (ret_qnodes_) {
			SyncDomain::QueueNode* qn = ret_qnodes_;
			ret_qnodes_ = qn->next ();
			qn->release ();
		}
	}

	static Core_var <ExecDomain> get (DeadlineTime deadline)
	{
		Scheduler::activity_begin ();	// Throws exception if shutdown was started.
		Core_var <ExecDomain> exec_domain = pool_.get ();
		exec_domain->deadline_ = deadline;
		return exec_domain;
	}

public:
	ExecDomain* wait_list_next_;

private:
	static ObjectPool <ExecDomain> pool_;

	DeadlineTime deadline_;
	SyncContext* sync_context_;
	SyncDomain::QueueNode* ret_qnodes_;
	Heap heap_;
	RuntimeSupportImpl runtime_support_;
	bool scheduler_item_created_;
	CORBA::Exception::Code scheduler_error_;

	class ReleaseToPool : public Runnable // Runnable for return object to pool in neutral context.
	{
	public:
		virtual void run ();
		ExecDomain* obj_;
	};

	ImplStatic <ReleaseToPool> release_to_pool_;

	void* runnable_space_ [MAX_RUNNABLE_SIZE];
};

}
}

#endif
