// Nirvana project.
// Execution domain (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "ExecContext.h"
#include "Thread.h"
#include "Scheduler.h"
#include "Runnable.h"
#include "ObjectPool.h"
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
	static void async_call (Runnable& runnable, DeadlineTime deadline, SyncDomain* sync_domain);

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

	void execute (DeadlineTime deadline)
	{
		assert (deadline_ == deadline);
		Thread::current ().execution_domain (this);
		ExecContext::switch_to ();
	}

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
		heap_.cleanup ();
	}

	void execute_loop ();

	SyncDomain* cur_sync_domain () const
	{
		return cur_sync_domain_;
	}

	Heap& heap ()
	{
		return heap_;
	}

private:
	static class Release :
		public ImplStatic <Runnable>
	{
	public:
		void run ();
	} release_;

	static class Schedule :
		public ImplStatic <Runnable>
	{
	public:
		void run ();
	} schedule_;

	void schedule_internal ();

private:
	DeadlineTime deadline_;
	SyncDomain* cur_sync_domain_;
	Heap heap_;
};

}
}

#endif
