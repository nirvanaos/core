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

class ExecDomain :
	public CoreObject,
	public ExecContext,
	public Executor
{
public:
	static void async_call (Runnable& runnable, DeadlineTime deadline, SyncDomain* sync_domain, CORBA::Nirvana::EnvironmentBridge* environment);

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
		cur_sync_domain_ (nullptr)
	{}

	//! Constructor with parameters can be used in porting for special cases.
	template <class ... Args>
	ExecDomain (Args ... args) :
		ExecContext (std::forward <Args> (args)...),
		wait_list_next_ (nullptr),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		cur_sync_domain_ (nullptr)
	{
		Scheduler::activity_begin ();
	}

	DeadlineTime deadline () const
	{
		return deadline_;
	}

	void schedule (SyncDomain* sync_domain);

	void execute (DeadlineTime deadline);

	template <class Starter>
	static void start (Core_var <ExecDomain>& exec_domain, Runnable& runnable, DeadlineTime deadline,
		SyncDomain* sync_domain, CORBA::Nirvana::EnvironmentBridge* environment, Starter starter)
	{
		exec_domain->runnable_ = &runnable;
		exec_domain->environment_ = environment;
		exec_domain->deadline_ = deadline;
		exec_domain->cur_sync_domain_ = sync_domain;
		starter ();
		exec_domain.detach (); // Object will be released when work is done.
	}

	void suspend ();
	
	inline void resume ()
	{
		schedule_internal ();
	}

	void _activate ()
	{}

	void _deactivate ()
	{
		assert (ExecContext::current () != this);
		Scheduler::activity_end ();
		runnable_.reset ();
		runtime_support_.cleanup ();
		heap_.cleanup ();
	}

	void execute_loop ();

	void on_crash ()
	{
		ExecContext::on_crash ();
		run_in_neutral_context (release_);
	}

	SyncDomain* cur_sync_domain () const
	{
		return cur_sync_domain_;
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
	class Release :
		public Runnable
	{
	public:
		void run ();
	};

	class Schedule :
		public Runnable
	{
	public:
		void run ();
	};

	void schedule_internal ();

public:
	void* wait_list_next_;

private:
	static ImplStatic <Release> release_;
	static ImplStatic <Schedule> schedule_;
	static ObjectPool <ExecDomain> pool_;

	DeadlineTime deadline_;
	SyncDomain* cur_sync_domain_;
	Heap heap_;
	RuntimeSupportImpl runtime_support_;
};

}
}

#endif
