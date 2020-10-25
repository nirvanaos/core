// Nirvana project.
// Execution domain (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "ExecContext.h"
#include "Scheduler.h"
#include "Runnable.h"
#include "ObjectPool.h"
#include "RuntimeSupportImpl.h"
#include <limits>
#include <utility>

namespace Nirvana {
namespace Core {

class SyncDomain;

class ExecDomain :
	public ExecContext,
	public ImplPoolable <ExecDomain, Executor>
{
public:
	static void async_call (Runnable& runnable, DeadlineTime deadline, SyncDomain* sync_domain, CORBA::Nirvana::EnvironmentBridge* environment);

	ExecDomain () :
		ExecContext (),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		cur_sync_domain_ (nullptr)
	{}

	//! Constructor with parameters can be used in porting for special cases.
	template <class ... Args>
	ExecDomain (Args ... args) :
		ExecContext (std::forward <Args> (args)...),
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

	void suspend ();
	
	inline void resume ()
	{
		schedule_internal ();
	}

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
	
	static ImplStatic <Release> release_;

	class Schedule :
		public Runnable
	{
	public:
		void run ();
	};
	
	static ImplStatic <Schedule> schedule_;

	void schedule_internal ();

private:
	DeadlineTime deadline_;
	SyncDomain* cur_sync_domain_;
	Heap heap_;
	RuntimeSupportImpl runtime_support_;
};

}
}

#endif
