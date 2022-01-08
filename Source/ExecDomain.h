/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "SyncDomain.h"
#include "Scheduler.h"
#include "ObjectPool.h"
#include "CoreObject.h"
#include "RuntimeSupportImpl.h"
#include "UserAllocator.h"
#include "RuntimeGlobal.h"
#include <limits>
#include <utility>
#include "parallel-hashmap/parallel_hashmap/phmap.h"

namespace Nirvana {
namespace Core {

/// Execution domain (coroutine, fiber).
class NIRVANA_NOVTABLE ExecDomain :
	public CoreObject,
	public ExecContext,
	public Executor
{
public:
	typedef ImplPoolable <ExecDomain> Impl;

	static void initialize ()
	{
		suspend_.construct ();
	}

	static void terminate () NIRVANA_NOEXCEPT
	{}

	static void async_call (const DeadlineTime& deadline, Runnable& runnable, SyncContext& sync_context)
	{
		CoreRef <ExecDomain> exec_domain = get (deadline);
		exec_domain->runnable_ = &runnable;
		exec_domain->spawn (sync_context);
	}

	/// Used in porting for creation of the startup domain.
	template <class ... Args>
	static CoreRef <ExecDomain> create_main (const DeadlineTime& deadline, Runnable& startup, Args ... args)
	{
		return CoreRef <ExecDomain>::create <ImplPoolable <ExecDomain> > (deadline, std::ref (startup), std::forward <Args> (args)...);
	}

	/// Get execution domain for a background thread.
	static CoreRef <ExecDomain> get_background (SyncContext& sync_context, Runnable& startup)
	{
		CoreRef <ExecDomain> exec_domain = get (INFINITE_DEADLINE);
		exec_domain->runnable_ = &startup;
		exec_domain->sync_context_ = &sync_context;
		return exec_domain;
	}

	DeadlineTime deadline () const
	{
		return deadline_;
	}

	void deadline (const DeadlineTime& dt)
	{
		deadline_ = dt;
	}

	void schedule_call (SyncContext& sync_context);
	void schedule_return (SyncContext& sync_context) NIRVANA_NOEXCEPT;

	/// Schedules this ED to execute.
	/// Must be called from another execution context.
	///
	/// \param sync_domain Synchronization domain. May be `nullptr`.
	void schedule (SyncContext& sync_context);

	/// Executor::execute ()
	/// Called from worker thread.
	void execute (int scheduler_error);

	template <class Starter>
	void start (Starter starter)
	{
		assert (runnable_);
		starter ();
		_add_ref ();
	}

	void spawn (SyncContext& sync_context);

	/// Suspend
	void suspend ();

	/// Resume suspended domain
	void resume ()
	{
		assert (ExecContext::current_ptr () != this);
		assert (sync_context_);
		sync_context_->schedule_return (*this);
	}

	void _activate ()
	{}

	/// Clear data and return object to the pool
	void _deactivate (ImplPoolable <ExecDomain>& obj) NIRVANA_NOEXCEPT
	{
		assert (!runnable_);
		assert (!sync_context_);
#ifdef _DEBUG
		Thread& thread = Thread::current ();
		assert (thread.exec_domain () != this);
#endif
		Scheduler::activity_end ();
		if (&ExecContext::current () == this)
			run_in_neutral_context (release_to_pool_);
		else
			pool_.release (obj);
	}

	/// \brief Called from the Port implementation.
	void execute_loop () NIRVANA_NOEXCEPT;

	/// Called from the Port implementation in case of the unrecoverable system error.
	/// \param err Exception code.
	void on_exec_domain_crash (CORBA::SystemException::Code err) NIRVANA_NOEXCEPT;

	SyncContext& sync_context () const NIRVANA_NOEXCEPT
	{
		assert (sync_context_);
		return *sync_context_;
	}

	void sync_context (SyncContext& sync_context) NIRVANA_NOEXCEPT
	{
		sync_context_ = &sync_context;
	}

	/// \returns Current heap.
	Heap& heap () NIRVANA_NOEXCEPT
	{
		return *cur_heap_;
	}

	/// Used to set module heap before the module initialization.
	void heap_replace (Heap& heap) NIRVANA_NOEXCEPT
	{
		cur_heap_ = &heap;
	}

	/// Used to restore heap after the module initialization.
	void heap_restore () NIRVANA_NOEXCEPT
	{
		cur_heap_ = &heap_;
	}

	RuntimeSupport& runtime_support () NIRVANA_NOEXCEPT
	{
		return runtime_support_;
	}

	CORBA::Exception::Code scheduler_error () const NIRVANA_NOEXCEPT
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
		Scheduler::activity_begin ();
		ctor_base ();
		deadline_ = deadline;
		runnable_ = &startup;
	}

private:
	void ctor_base ();

	void ret_qnodes_clear () NIRVANA_NOEXCEPT
	{
		while (ret_qnodes_) {
			SyncDomain::QueueNode* qn = ret_qnodes_;
			ret_qnodes_ = qn->next ();
			qn->release ();
		}
	}

	static CoreRef <ExecDomain> get (DeadlineTime deadline);

	void cleanup () NIRVANA_NOEXCEPT;

private:
	class NeutralOp : public ImplStatic <Runnable>
	{
	public:
		void init (ExecDomain& ed)
		{
			exec_domain_ = &ed;
		}

	protected:
		ExecDomain* exec_domain_;
	};

	class ReleaseToPool : public NeutralOp
	{
	private:
		virtual void run ();
	};

	class ScheduleCall : public NeutralOp
	{
	public:
		SyncContext* sync_context_;
		std::exception_ptr exception_;

	private:
		virtual void run ();
		virtual void on_exception () NIRVANA_NOEXCEPT;
	};

	class ScheduleReturn : public NeutralOp
	{
	public:
		SyncContext* sync_context_;

	private:
		virtual void run ();
	};

	class Suspend : public ImplStatic <Runnable>
	{
	private:
		virtual void run ();
	};

public:
	enum class RestrictedMode
	{
		NO_RESTRICTIONS,
		CLASS_LIBRARY_INIT,
		MODULE_TERMINATE
	}
	restricted_mode_;

	/// Used by CORBA::Internal::ObjectFactory
	void* stateless_creation_frame_;

	/// Used by Binder
	void* binder_context_;

	/// Run-time global state
	RuntimeGlobal runtime_global_;

private:
	static ObjectPool <ExecDomain> pool_;
	static StaticallyAllocated <Suspend> suspend_;

	DeadlineTime deadline_;
	CoreRef <SyncContext> sync_context_;
	SyncDomain::QueueNode* ret_qnodes_;
	HeapUser heap_;
	Heap* cur_heap_;
	RuntimeSupportImpl <UserAllocator> runtime_support_;
	bool scheduler_item_created_;
	CORBA::Exception::Code scheduler_error_;
	ReleaseToPool release_to_pool_;
	ScheduleCall schedule_call_;
	ScheduleReturn schedule_return_;
};

}
}

#endif
