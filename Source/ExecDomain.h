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
#pragma once

#include "SyncDomain.h"
#include "Scheduler.h"
#include "Stack.h"
#include "RuntimeGlobal.h"
#include "PreallocatedStack.h"
#include "MemContextUser.h"
#include "ObjectPool.h"
#include "ThreadBackground.h"
#include <limits>
#include <utility>
#include <signal.h>

namespace Nirvana {

namespace Legacy {
namespace Core {
class Process;
class ThreadBase;
}
}

namespace Core {

/// Execution domain (coroutine, fiber).
class ExecDomain final :
	public SharedObject,
	public ExecContext,
	public Executor,
	public StackElem
{
public:
	/// \returns Current execution domain.
	static ExecDomain& current () NIRVANA_NOEXCEPT
	{
		ExecDomain* ed = Thread::current ().exec_domain ();
		assert (ed);
		return *ed;
	}

	/// Asynchronous call.
	/// 
	/// \param deadline    Deadline.
	/// \param runnable    Runnable object to execute.
	/// \param target      Target Synchronization context.
	/// \param mem_context Target memory context (optional).
	static void async_call (const DeadlineTime& deadline, CoreRef <Runnable> runnable,
		SyncContext& target, MemContext* mem_context = nullptr);

	/// Start legacy process.
	/// 
	/// \param process   The Process object.
	static void start_legacy_process (Legacy::Core::Process& process);

	DeadlineTime deadline () const
	{
		return deadline_;
	}

	void deadline (const DeadlineTime& dt)
	{
		deadline_ = dt;
	}

	// Made inline because the only one call in Synchronized constructor.
	void schedule_call (SyncContext& target, MemContext* mem_context)
	{
		SyncDomain* sd = target.sync_domain ();
		if (sd) {
			assert (!mem_context || mem_context == &sd->mem_context ());
			mem_context = &sd->mem_context ();
		}
		mem_context_push (mem_context);

		SyncContext& old_context = sync_context ();

		// If old context is a synchronization domain, we
		// allocate queue node to perform return without a risk
		// of memory allocation failure.
		SyncDomain* old_sd = old_context.sync_domain ();
		if (old_sd) {
			ret_qnode_push (*old_sd);
			old_sd->leave ();
		}

		if (sd || !(deadline () == INFINITE_DEADLINE && &Thread::current () == background_worker_)) {
			// Need to schedule

			// Call schedule() in the neutral context
			schedule_.sync_context_ = &target;
			schedule_.ret_ = false;
			run_in_neutral_context (schedule_);

			// Handle possible schedule() exceptions
			if (schedule_.exception_) {
				std::exception_ptr ex = schedule_.exception_;
				schedule_.exception_ = nullptr;
				// We leaved old sync domain so we must enter into prev synchronization domain back before throwing the exception.
				if (old_sd)
					schedule_return (old_context);
				rethrow_exception (ex);
			}

			// Now ED in the scheduler queue.
			// Now ED is again executed by the scheduler.
			// But maybe a schedule error occurs.
			CORBA::Exception::Code err = scheduler_error ();
			if (err >= 0) {
				// We must return to prev synchronization context back before throwing the exception.
				schedule_return (old_context);
				CORBA::SystemException::_raise_by_code (err);
			}
		} else
			sync_context (target);
	}

	void schedule_return (SyncContext& target) NIRVANA_NOEXCEPT;

	/// Suspend execution
	/// 
	/// \param resume_context Context where to resume or nullptr for current context.
	void suspend (SyncContext* resume_context = nullptr);

	/// Resume suspended domain
	void resume ()
	{
		assert (ExecContext::current_ptr () != this);
		assert (sync_context_);
		schedule (*sync_context_, true);
	}

	/// Reschedule
	static bool yield ();

	/// \brief Called from the Port implementation.
	void run () NIRVANA_NOEXCEPT;

	/// Called from the Port implementation in case of the unrecoverable error.
	/// \param signal The signal information.
	void on_crash (const siginfo_t& signal) NIRVANA_NOEXCEPT;

	/// Called from the Port implementation in case of the signal.
	/// \param signal The signal information.
	/// 
	/// \returns `true` to continue execution,
	///          `false` to unwind and call on_crash().
	bool on_signal (const siginfo_t& signal)
	{
		if (signal.si_excode != CORBA::Exception::EC_NO_EXCEPTION) {
			sync_context ().raise_exception ((CORBA::SystemException::Code)signal.si_excode, signal.si_code);
			return true;
		}

		return false;
	}

	SyncContext& sync_context () const NIRVANA_NOEXCEPT
	{
		assert (sync_context_);
		return *sync_context_;
	}

	void sync_context (SyncContext& sync_context) NIRVANA_NOEXCEPT
	{
		sync_context_ = &sync_context;
	}

	/// \returns Current memory context.
	MemContext& mem_context ()
	{
		if (!mem_context_) {
			mem_context_ = 
				mem_context_stack_.top () = MemContextUser::create ();
		}
		return *mem_context_;
	}

	MemContext* mem_context_ptr () NIRVANA_NOEXCEPT
	{
		return mem_context_;
	}

	/// Push new memory context.
	void mem_context_push (MemContext* context)
	{
		mem_context_stack_.emplace (context);
		mem_context_ = context;
#ifdef _DEBUG
		++dbg_context_stack_size_;
#endif
	}

	/// Pop memory context stack.
	void mem_context_pop () NIRVANA_NOEXCEPT
	{
		mem_context_stack_.pop ();
#ifdef _DEBUG
		--dbg_context_stack_size_;
#endif
		mem_context_ = mem_context_stack_.top ();
	}

	CORBA::Exception::Code scheduler_error () const NIRVANA_NOEXCEPT
	{
		return scheduler_error_;
	}

#ifdef _DEBUG
	size_t dbg_context_stack_size_;
#endif
	enum class RestrictedMode
	{
		NO_RESTRICTIONS,
		CLASS_LIBRARY_INIT,
		MODULE_TERMINATE
	};

	RestrictedMode restricted_mode () const NIRVANA_NOEXCEPT
	{
		return restricted_mode_;
	}

	void restricted_mode (RestrictedMode rm) NIRVANA_NOEXCEPT
	{
		restricted_mode_ = rm;
	}

	/// Run-time global state
	RuntimeGlobal runtime_global_;

	/// Called on core startup.
	static void initialize ();
	
	/// Called on core shutdown.
	static void terminate () NIRVANA_NOEXCEPT;

private:
	ExecDomain () :
		ExecContext (false),
#ifdef _DEBUG
		dbg_context_stack_size_ (0),
#endif
		ref_cnt_ (1),
		ret_qnodes_ (nullptr),
		scheduler_item_created_ (false),
		scheduler_error_ (CORBA::SystemException::EC_NO_EXCEPTION),
		schedule_ (*this),
		yield_ (*this),
		deleter_ (CoreRef <Runnable>::create <ImplDynamic <Deleter> > (std::ref (*this))),
		restricted_mode_ (RestrictedMode::NO_RESTRICTIONS)
	{}

	class WithPool;
	class NoPool;

	using Creator = std::conditional <EXEC_DOMAIN_POOLING, WithPool, NoPool>::type;

	static CoreRef <ExecDomain> create (const DeadlineTime deadline, CoreRef <Runnable> runnable, MemContext* mem_context = nullptr);

	~ExecDomain () NIRVANA_NOEXCEPT
	{}

	void final_release () NIRVANA_NOEXCEPT;

	friend class CoreRef <ExecDomain>;
	friend class ObjectPool <ExecDomain>;
	void _add_ref () NIRVANA_NOEXCEPT;
	void _remove_ref () NIRVANA_NOEXCEPT;
	void cleanup () NIRVANA_NOEXCEPT;

	void spawn (SyncContext& sync_context);

	/// Schedules this ED to execute.
	/// Must be called from another execution context.
	///
	/// \param sync_domain Synchronization domain. May be `nullptr`.
	/// \param ret `true` if scheduled return from call.
	void schedule (SyncContext& sync_context, bool ret = false);

	/// Executor::execute ()
	/// Called from worker thread.
	void execute (int scheduler_error);

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

	void ret_qnodes_clear () NIRVANA_NOEXCEPT
	{
		while (ret_qnodes_) {
			SyncDomain::QueueNode* qn = ret_qnodes_;
			ret_qnodes_ = qn->next ();
			qn->release ();
		}
	}

	void unwind_mem_context () NIRVANA_NOEXCEPT;

	static void start_legacy_thread (Legacy::Core::Process& process, Legacy::Core::ThreadBase& thread);

private:
	class Schedule : public ImplStatic <Runnable>
	{
	public:
		Schedule (ExecDomain& ed) :
			exec_domain_ (ed)
		{}

		SyncContext* sync_context_;
		std::exception_ptr exception_;
		bool ret_;

	private:
		virtual void run ();
		virtual void on_exception () NIRVANA_NOEXCEPT;

	private:
		ExecDomain& exec_domain_;
	};

	class NIRVANA_NOVTABLE Deleter : public Runnable
	{
	protected:
		Deleter (ExecDomain& ed) :
			exec_domain_ (ed)
		{}

	private:
		virtual void run ();
	
	private:
		ExecDomain& exec_domain_;
	};

	class Suspend : public ImplStatic <Runnable>
	{
	private:
		virtual void run ();
	};

	class Yield : public ImplStatic <Runnable>
	{
	public:
		Yield (ExecDomain& ed) :
			exec_domain_ (ed)
		{}
	private:
		virtual void run ();

	private:
		ExecDomain& exec_domain_;
	};

private:
	static StaticallyAllocated <Suspend> suspend_;

	AtomicCounter <false> ref_cnt_;
	DeadlineTime deadline_;
	CoreRef <SyncContext> sync_context_;
	SyncDomain::QueueNode* ret_qnodes_;
	
	PreallocatedStack <CoreRef <MemContext> > mem_context_stack_;
	// When we perform mem_context_stack_.pop (), the top memory context is still
	// current. This is necessary for correct memory deallocations in MemContext
	// destructor.
	MemContext* mem_context_;

	bool scheduler_item_created_;
	CORBA::Exception::Code scheduler_error_;
	Schedule schedule_;
	Yield yield_;
	CoreRef <Runnable> deleter_;
	CoreRef <ThreadBackground> background_worker_;
	RestrictedMode restricted_mode_;
};

}
}

#endif
