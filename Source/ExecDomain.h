// Nirvana project.
// Execution domain (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "ExecContext.h"
#include "Scheduler.h"
#include "Runnable.h"
#include "ObjectPool.h"
#include "CoreObject.h"
#include "RuntimeSupportImpl.h"
#include <limits>
#include <utility>

namespace Nirvana {
namespace Core {

class SyncDomain;

class NIRVANA_NOVTABLE ExecDomain :
	public CoreObject,
	public ExecContext,
	public Executor
{
public:
	static void async_call (Runnable& runnable, DeadlineTime deadline, SyncDomain* sync_domain, CORBA::Nirvana::Interface_ptr environment);

	static Core_var <ExecDomain> get ()
	{
		Scheduler::activity_begin ();	// Throws exception if shutdown was started.
		return pool_.get ();
	}

	template <class ... Args>
	static Core_var <ExecDomain> create (Args ... args)
	{
		return Core_var <ExecDomain>::create <ImplPoolable <ExecDomain> > (std::ref (pool_), std::forward <Args> (args)...);
	}

	ExecDomain () :
		ExecContext (),
		wait_list_next_ (nullptr),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		sync_domain_ (nullptr),
		scheduler_error_ (CORBA::SystemException::EC_NO_EXCEPTION)
	{}

	//! Constructor with parameters can be used in porting for special cases.
	template <class ... Args>
	ExecDomain (Args ... args) :
		ExecContext (std::forward <Args> (args)...),
		wait_list_next_ (nullptr),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		sync_domain_ (nullptr)
	{
		Scheduler::activity_begin ();
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
	void execute (DeadlineTime deadline, Word scheduler_error);

	template <class Starter>
	void start (Runnable& runnable, DeadlineTime deadline, SyncDomain* sync_domain,
		CORBA::Nirvana::Interface_ptr environment, Starter starter)
	{
		runnable_ = &runnable;
		environment_ = environment;
		deadline_ = deadline;
		sync_domain_ = sync_domain;
		starter ();
		_add_ref ();
	}

	void suspend ();
	
	inline void resume ()
	{
		assert (&ExecContext::current () != this);
		schedule (sync_domain_, true);
	}

	void _activate ()
	{}

	void _deactivate ()
	{
		assert (&ExecContext::current () != this);
		Scheduler::activity_end ();
		runnable_.reset ();
		sync_domain_ = nullptr;
		scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
		runtime_support_.cleanup ();
		heap_.cleanup ();
	}

	void execute_loop ();

	void on_crash ()
	{
		ExecContext::on_crash ();
		release ();
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

private:
	class NIRVANA_NOVTABLE Release :
		public Runnable
	{
	public:
		void run ();
	};

	void release ()
	{
		run_in_neutral_context (release_, CORBA::Nirvana::Interface::_nil ());
	}

	void check_schedule_error ();

public:
	ExecDomain* wait_list_next_;

private:
	static ImplStatic <Release> release_;
	static ObjectPool <ExecDomain> pool_;

	DeadlineTime deadline_;
	SyncDomain* sync_domain_;
	Word scheduler_error_;
	Heap heap_;
	RuntimeSupportImpl runtime_support_;
};

}
}

#endif
