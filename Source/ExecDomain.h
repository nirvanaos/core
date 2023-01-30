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
#include "ObjectPool.h"
#include "ThreadBackground.h"
#include "CoreObject.h"
#include "unrecoverable_error.h"
#include <limits>
#include <utility>

struct siginfo;

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
	public CoreObject, // Execution domains must be created quickly.
	public ExecContext,
	public Executor,
	public StackElem
{
public:
	static const size_t MAX_RUNNABLE_SIZE = 2 * sizeof (void*) + 20;

	/// \returns Current execution domain.
	static ExecDomain& current () NIRVANA_NOEXCEPT
	{
		ExecDomain* ed = Thread::current ().exec_domain ();
		assert (ed);
		return *ed;
	}

	/// Asynchronous call.
	///
	/// \tparam R Runnable class
	/// \param deadline Deadline.
	/// \param target   Target Synchronization context.
	/// \param heap     Shared heap (optional).
	/// \param args     Arguments for R constructor.
	/// 
	template <class R, class ... Args>
	static void async_call (const DeadlineTime& deadline,
		SyncContext& target, Heap* heap, Args ... args)
	{
		async_call <R, Args...> (deadline, target, get_mem_context (target, heap), std::forward <Args> (args)...);
	}

	/// \brief Asynchronous call.
	///
	/// \tparam R Runnable class
	/// \param deadline    Deadline.
	/// \param target      Target Synchronization context.
	/// \param mem_context Memory context.
	/// \param args        Arguments for R constructor.
	/// 
	template <class R, class ... Args>
	static void async_call (const DeadlineTime& deadline,
		SyncContext& target, Ref <MemContext>&& mem_context, Args ... args)
	{
		static_assert (sizeof (R) <= MAX_RUNNABLE_SIZE, "Runnable too large");

#ifdef _DEBUG
		{
			SyncDomain* sd = target.sync_domain ();
			assert (!sd || &sd->mem_context () == mem_context);
		}
#endif

		Ref <ExecDomain> exec_domain = create (deadline, std::move (mem_context));
		exec_domain->runnable_ = new (&exec_domain->runnable_space_) R (std::forward <Args> (args)...);
		exec_domain->spawn (target);
	}

	/// Asynchronous call.
	/// 
	/// \param deadline Deadline.
	/// \param runnable Pointer to the Runnable object.
	/// \param target   Target Synchronization context.
	/// \param heap     Shared heap (optional).
	static void async_call (const DeadlineTime& deadline, Runnable* runnable,
		SyncContext& target, Heap* heap);

	/// Start legacy process.
	/// 
	/// \param process The Process object.
	static void start_legacy_process (Legacy::Core::Process& process);

	const DeadlineTime& deadline () const NIRVANA_NOEXCEPT
	{
		return deadline_;
	}

	void deadline (const DeadlineTime& dt) NIRVANA_NOEXCEPT
	{
		deadline_ = dt;
	}

	const System::DeadlinePolicy& deadline_policy_async () const NIRVANA_NOEXCEPT
	{
		return deadline_policy_async_;
	}

	void deadline_policy_async (const System::DeadlinePolicy& dp) NIRVANA_NOEXCEPT
	{
		deadline_policy_async_ = dp;
	}

	const System::DeadlinePolicy& deadline_policy_oneway () const NIRVANA_NOEXCEPT
	{
		return deadline_policy_oneway_;
	}

	void deadline_policy_oneway (const System::DeadlinePolicy& dp) NIRVANA_NOEXCEPT
	{
		deadline_policy_oneway_ = dp;
	}

	DeadlineTime get_request_deadline (bool oneway) const NIRVANA_NOEXCEPT;

	/// Schedule a call to a synchronization context.
	/// Made inline because it is called only once in Synchronized constructor.
	/// 
	/// \param target The target sync context.
	/// \param heap (optional) The heap pointer for the memory context creation.
	void schedule_call (SyncContext& target, Heap* heap)
	{
		mem_context_push (get_mem_context (target, heap));
		schedule_call_no_push_mem (target);
	}

	/// Schedule a call to a synchronization context.
	/// Made inline because it is called only once in Synchronized constructor.
	/// 
	/// \param target The target sync context.
	/// \param mem_context The memory context.
	void schedule_call (SyncContext& target, Ref <MemContext>&& mem_context)
	{
#ifdef _DEBUG
		{
			SyncDomain* sd = target.sync_domain ();
			assert (!sd || &sd->mem_context () == mem_context);
		}
#endif
		mem_context_push (std::move (mem_context));
		schedule_call_no_push_mem (target);
	}

	void schedule_call_no_push_mem (SyncContext& target);

	/// Schedule return
	/// \param target The target sync context.
	/// \param no_reschedule (optional) Do not re-schedule if the target context is the same
	///        as current.
	void schedule_return (SyncContext& target, bool no_reschedule = false) NIRVANA_NOEXCEPT;

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
	static bool reschedule ();

	/// \brief Called from the Port implementation.
	void run ();

	/// Called from the Port implementation in case of the unrecoverable error.
	/// \param signal The signal information.
	void on_crash (const siginfo& signal) NIRVANA_NOEXCEPT
	{
		// Leave sync domain if one.
		SyncDomain* sd = sync_context_->sync_domain ();
		if (sd)
			sd->leave ();
		sync_context_ = &g_core_free_sync_context;

		unwind_mem_context ();

		if (runnable_) {
			runnable_->on_crash (signal);
			runnable_ = nullptr;
		} else
			unrecoverable_error (signal.si_signo);

		cleanup ();
	}

	/// Called from the Port implementation in case of the signal.
	/// \param signal The signal information.
	/// 
	/// \returns `true` to continue execution,
	///          `false` to unwind and call on_crash().
	bool on_signal (const siginfo_t& signal)
	{
		// TODO: Check for the signal handlers and return true if signal is handled.
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

	/// Leave the current sync domain, if any.
	void leave_sync_domain () NIRVANA_NOEXCEPT
	{
		SyncDomain* sd = sync_context ().sync_domain ();
		if (sd) {
			sync_context (g_core_free_sync_context);
			sd->leave ();
		}
	}

	/// \brief Returns current memory context.
	/// 
	/// If context is `nullptr`, the new user context will be created.
	/// 
	/// \returns Current memory context.
	MemContext& mem_context ();

	/// \brief Returns current memory context pointer.
	///
	/// \returns Current memory context pointer which can be `nullptr`.
	MemContext* mem_context_ptr () const NIRVANA_NOEXCEPT
	{
		return mem_context_;
	}

	/// \brief Swap current memory context.
	/// 
	/// \param other Other memory context
	void mem_context_swap (Ref <MemContext>& other) NIRVANA_NOEXCEPT
	{
		mem_context_ = other;
		mem_context_stack_.top ().swap (other);
	}

	/// \brief Temporary replace current memory context.
	/// 
	/// Current memory context then must be restored by call mem_context_restore ().
	/// Use carefully. When memory context is temporary replaced, no context switch is allowed.
	/// 
	/// \param tmp Memory context for temporary replace.
	void mem_context_replace (MemContext& tmp) NIRVANA_NOEXCEPT
	{
		mem_context_ = &tmp;
	}

	/// \brief Restore the current memory context.
	/// 
	void mem_context_restore () NIRVANA_NOEXCEPT
	{
		if (mem_context_stack_.empty ())
			mem_context_ = nullptr;
		else
			mem_context_ = mem_context_stack_.top ();
	}

	/// Push new memory context.
	void mem_context_push (Ref <MemContext>&& mem_context)
	{
		MemContext* p = mem_context;
		mem_context_stack_.emplace (std::move (mem_context));
		mem_context_ = p;
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

#ifdef _DEBUG
	size_t dbg_context_stack_size_;
#endif
	enum class RestrictedMode
	{
		NO_RESTRICTIONS,
		CLASS_LIBRARY_INIT,
		MODULE_TERMINATE,
		SUPPRESS_ASYNC_GC
	};

	RestrictedMode restricted_mode () const NIRVANA_NOEXCEPT
	{
		return restricted_mode_;
	}

	void restricted_mode (RestrictedMode rm) NIRVANA_NOEXCEPT
	{
		restricted_mode_ = rm;
	}

	void get_context (const char* const* ids, size_t id_cnt, std::vector <std::string>& context)
	{
		// TODO: Implement
	}

	void set_context (std::vector <std::string>& context)
	{
		// TODO: Implement
	}

	/// Run-time global state
	RuntimeGlobal runtime_global_;

	/// Abort execution with SIGTERM signal.
	void abort ()
	{
		// TODO: Implement
		throw_NO_IMPLEMENT ();
	}

	/// Called on core startup.
	static void initialize () NIRVANA_NOEXCEPT;
	
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
		schedule_ (*this),
		reschedule_ (*this),
		deleter_ (*this),
		restricted_mode_ (RestrictedMode::NO_RESTRICTIONS)
	{
		deadline_policy_oneway_._d (System::DeadlinePolicyType::DEADLINE_INFINITE);
	}

	class WithPool;
	class NoPool;

	using Creator = std::conditional <EXEC_DOMAIN_POOLING, WithPool, NoPool>::type;

	static Ref <ExecDomain> create (const DeadlineTime deadline, Ref <MemContext>&& mem_context);
	static Ref <MemContext> get_mem_context (SyncContext& target, Heap* heap = nullptr);

	static Ref <ExecDomain> create (const DeadlineTime deadline, SyncContext& target, Heap* heap = nullptr)
	{
		return create (deadline, get_mem_context (target, heap));
	}

	~ExecDomain () NIRVANA_NOEXCEPT
	{}

	void final_release () NIRVANA_NOEXCEPT;

	friend class Ref <ExecDomain>;
	template <class> friend class CORBA::servant_reference;
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
	virtual void execute () override;

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

	inline void unwind_mem_context () NIRVANA_NOEXCEPT
	{
		// Clear memory context stack
		Ref <MemContext> tmp;
		do {
			tmp = std::move (mem_context_stack_.top ());
			mem_context_stack_.pop ();
			mem_context_ = tmp;
		} while (!mem_context_stack_.empty ());
		mem_context_stack_.push (std::move (tmp));
	}

	static void start_legacy_thread (Legacy::Core::Process& process, Legacy::Core::ThreadBase& thread);

	void create_background_worker ();

private:
	class Schedule : public Runnable
	{
	public:
		Schedule (ExecDomain& ed) :
			exec_domain_ (ed)
		{}

		SyncContext* sync_context_;
		bool ret_;

	private:
		virtual void run ();

	private:
		ExecDomain& exec_domain_;
	};

	class Deleter : public Runnable
	{
	public:
		Deleter (ExecDomain& ed) :
			exec_domain_ (ed)
		{}

	private:
		virtual void run ();
	
	private:
		ExecDomain& exec_domain_;
	};

	class Reschedule : public Runnable
	{
	public:
		Reschedule (ExecDomain& ed) :
			exec_domain_ (ed)
		{}

	private:
		virtual void run ();

	private:
		ExecDomain& exec_domain_;
	};

private:
	AtomicCounter <false> ref_cnt_;
	DeadlineTime deadline_;
	Ref <SyncContext> sync_context_;
	SyncDomain::QueueNode* ret_qnodes_;
	
	PreallocatedStack <Ref <MemContext> > mem_context_stack_;
	// When we perform mem_context_stack_.pop (), the top memory context is still
	// current. This is necessary for correct memory deallocations in MemContext
	// destructor.
	MemContext* mem_context_;

	bool scheduler_item_created_;
	Schedule schedule_;
	Reschedule reschedule_;
	Deleter deleter_;
	Ref <ThreadBackground> background_worker_;
	RestrictedMode restricted_mode_;

	System::DeadlinePolicy deadline_policy_async_;
	System::DeadlinePolicy deadline_policy_oneway_;

	typename std::aligned_storage <MAX_RUNNABLE_SIZE>::type runnable_space_;
};

inline
MemContext& MemContext::current ()
{
	return ExecDomain::current ().mem_context ();
}

}
}

#endif
