// Nirvana project.
// Execution domain (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "ExecContext.h"
#include "ObjectPool.h"
#include "Thread.h"
#include "Scheduler_s.h"
#include <Nirvana/Runnable_s.h>
#include <limits>

namespace Nirvana {
namespace Core {

class SyncDomain;

class ExecDomain :
	public ExecContext,
	public PoolableObject,
	public ::CORBA::Nirvana::Servant <ExecDomain, Executor>,
	public ::CORBA::Nirvana::LifeCycleRefCntAbstract <ExecDomain>
{
public:
	static void async_call (Runnable_ptr runnable, DeadlineTime deadline, SyncDomain* sync_domain);

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

	ExecDomain () :
		ExecContext (),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		cur_sync_domain_ (nullptr)
	{}

	template <class P>
	ExecDomain (P param) :
		ExecContext (param),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		cur_sync_domain_ (nullptr)
	{}

	void activate ()
	{
		_add_ref ();
	}

	void _delete ()
	{
		assert (ExecContext::current () != this);
		pool_.release (*this);
	}

	void execute_loop ();

private:
	static ObjectPoolT <ExecDomain> pool_;

	class Release :
		public ::CORBA::Nirvana::ServantStatic <Release, Runnable>
	{
	public:
		static void run ()
		{
			Thread& thread = Thread::current ();
			thread.execution_domain ()->_remove_ref ();
			thread.execution_domain (nullptr);
		}
	};

	class Schedule :
		public ::CORBA::Nirvana::ServantStatic <Schedule, Runnable>
	{
	public:
		static void run ()
		{
			Thread& thread = Thread::current ();
			thread.execution_domain ()->schedule_internal ();
			thread.execution_domain (nullptr);
		}
	};

	void schedule_internal ();

	DeadlineTime deadline_;
	SyncDomain* cur_sync_domain_;
};

}
}

#endif
