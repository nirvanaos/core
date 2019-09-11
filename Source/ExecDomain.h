// Nirvana project.
// Execution domain (coroutine, fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "ExecContext.h"
#include "ObjectPool.h"
#include "Thread.h"
#include "Scheduler.h"
#include <Nirvana/Runnable_s.h>
#include <limits>

namespace Nirvana {
namespace Core {

class SyncDomain;

class ExecDomain :
	public PoolableObject,
	public ExecContext,
	public Executor,
	public ::CORBA::Nirvana::ServantTraits <ExecDomain>,
	public ::CORBA::Nirvana::LifeCycleRefCntPseudo <ExecDomain>
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

	void suspend ();
	
	inline void resume ()
	{
		schedule_internal ();
	}

	static void initialize ()
	{
		pool_.initialize ();
	}

	static void terminate ()
	{
		pool_.terminate ();
	}

	void activate ()
	{
		_add_ref ();
	}

	void _delete ()
	{
		assert (ExecContext::current () != this);
		pool_.release (*this);
		Scheduler::activity_end ();
	}

	void execute_loop ();

	//! Public constructor can be used in porting for special cases.
	template <class P>
	ExecDomain (P param) :
		ExecContext (param),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		cur_sync_domain_ (nullptr)
	{
		Scheduler::activity_begin ();
	}

private:
	friend class ObjectPool <ExecDomain>;

	ExecDomain () :
		ExecContext (),
		deadline_ (std::numeric_limits <DeadlineTime>::max ()),
		cur_sync_domain_ (nullptr)
	{}

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

private:
	DeadlineTime deadline_;
	SyncDomain* cur_sync_domain_;

	static ObjectPool <ExecDomain> pool_;
};

}
}

#endif
