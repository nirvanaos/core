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
#include "MemContextEx.h"
#include "ObjectPool.h"
#include "ThreadBackground.h"
#include <limits>
#include <utility>
#include <signal.h>

namespace Nirvana {

namespace Legacy {
namespace Core {
class Process;
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
	static void async_call (const DeadlineTime& deadline, Runnable& runnable,
		SyncContext& target, MemContext* mem_context = nullptr);

	/// Start legacy thread.
	/// 
	/// \param process   The Process object.
	/// \param runnable  Runnable object to execute.
	static void start_legacy_thread (Legacy::Core::Process& process, Runnable& runnable);

	DeadlineTime deadline () const
	{
		return deadline_;
	}

	void deadline (const DeadlineTime& dt)
	{
		deadline_ = dt;
	}

	void schedule_call (SyncContext& sync_context, MemContext* mem_context);
	void schedule_return (SyncContext& sync_context) NIRVANA_NOEXCEPT;

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

	/// Called from the Port implementation in case of the unrecoverable system error.
	/// \param signal The signal number.
	void on_crash (int signal) NIRVANA_NOEXCEPT;

	/// Called from the Port implementation in case of the signal.
	/// \param signal The signal number.
	/// \param minor The signal minor code.
	void on_signal (int signal, unsigned minor)
	{
		static const struct SigToExc
		{
			int signal;
			CORBA::Exception::Code ec;
		} sig2exc [] = {
			{ SIGILL, CORBA::SystemException::EC_ILLEGAL_INSTRUCTION },
			{ SIGFPE, CORBA::SystemException::EC_ARITHMETIC_ERROR },
			{ SIGSEGV, CORBA::SystemException::EC_ACCESS_VIOLATION }
		};

		for (const SigToExc* p = sig2exc; p != std::end (sig2exc); ++p) {
			if (p->signal == signal) {
				sync_context ().raise_exception (p->ec, minor);
				return;
			}
		}

		on_crash (signal);
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
		CoreRef <MemContext>& top = mem_context_.top ();
		if (!top)
			top = MemContextEx::create ();
		return *top;
	}

	CoreRef <MemContext>& mem_context_ptr () NIRVANA_NOEXCEPT
	{
		return mem_context_.top ();
	}

	/// Push new memory context.
	void mem_context_push (MemContext* context)
	{
		mem_context_.emplace (context);
#ifdef _DEBUG
		++dbg_context_stack_size_;
#endif
	}

	/// Pop memory context stack.
	void mem_context_pop () NIRVANA_NOEXCEPT
	{
		mem_context_.pop ();
#ifdef _DEBUG
		--dbg_context_stack_size_;
#endif
		assert (!mem_context_.empty ());
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
	}
	restricted_mode_;

	/// Used by CORBA::Internal::ObjectFactory
	void* stateless_creation_frame_;

	/// Used by Binder
	void* binder_context_;

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
		restricted_mode_ (RestrictedMode::NO_RESTRICTIONS),
		stateless_creation_frame_ (nullptr),
		binder_context_ (nullptr),
		ref_cnt_ (1),
		ret_qnodes_ (nullptr),
		scheduler_item_created_ (false),
		scheduler_error_ (CORBA::SystemException::EC_NO_EXCEPTION),
		schedule_ (*this),
		yield_ (*this),
		deleter_ (CoreRef <Runnable>::create <ImplDynamic <Deleter> > (std::ref (*this)))
	{}

	class WithPool;
	class NoPool;

	using Creator = std::conditional <EXEC_DOMAIN_POOLING, WithPool, NoPool>::type;

	static CoreRef <ExecDomain> create (const DeadlineTime deadline, Runnable& runnable, MemContext* mem_context = nullptr);

	void final_construct (const DeadlineTime& deadline, Runnable& runnable, MemContext* mem_context);

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
	PreallocatedStack <CoreRef <MemContext> > mem_context_;
	bool scheduler_item_created_;
	CORBA::Exception::Code scheduler_error_;
	Schedule schedule_;
	Yield yield_;
	CoreRef <Runnable> deleter_;
	CoreRef <ThreadBackground> background_worker_;
};

}
}

#endif
