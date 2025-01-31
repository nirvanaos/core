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
#include "pch.h"
#include "ExecDomain.h"
#include "Binder.h"
#include "Chrono.h"
#include "MemContext.h"
#include "DeadlinePolicy.h"
#include <Port/SystemInfo.h>

namespace Nirvana {
namespace Core {

class ExecDomain::WithPool
{
public:
	static void initialize () noexcept
	{
		// Set initial pool size with processor count.
		pool_.construct (Port::SystemInfo::hardware_concurrency ());
	}

	static void terminate () noexcept
	{
		pool_.destruct ();
	}

	static Ref <ExecDomain> create ()
	{
		return pool_->create ();
	}

	static void release (ExecDomain& ed) noexcept
	{
		pool_->release (ed);
	}

private:
	static StaticallyAllocated <ObjectPool <ExecDomain>> pool_;
};

StaticallyAllocated <ObjectPool <ExecDomain>> ExecDomain::WithPool::pool_;

class ExecDomain::NoPool
{
public:
	static void initialize () noexcept
	{}

	static void terminate () noexcept
	{}

	static Ref <ExecDomain> create ()
	{
		return Ref <ExecDomain>::create <ExecDomain> ();
	}

	static void release (ExecDomain& ed) noexcept
	{
		delete& ed;
	}
};

StaticallyAllocated <ExecDomain::Deleter> ExecDomain::deleter_;
StaticallyAllocated <ExecDomain::Reschedule> ExecDomain::reschedule_;

void ExecDomain::initialize () noexcept
{
	Creator::initialize ();

	deleter_.construct ();
	reschedule_.construct ();
}

void ExecDomain::terminate () noexcept
{
	Creator::terminate ();
}

Ref <ExecDomain> ExecDomain::create (const DeadlineTime deadline, Ref <MemContext>&& mem_context)
{
	Ref <ExecDomain> ed = Creator::create ();
	Scheduler::activity_begin ();
	try {
		ed->deadline_ = deadline;
		assert (ed->mem_context_stack_.empty ());

		MemContext* pmc = mem_context;
		ed->mem_context_stack_.push (std::move (mem_context));
		ed->mem_context_ = pmc;

		Thread* th = Thread::current_ptr ();
		if (th) {
			ExecDomain* parent = th->exec_domain ();
			if (parent)
				ed->impersonation_context_ = parent->impersonation_context_;
		}

#ifndef NDEBUG
		assert (!ed->dbg_mem_context_stack_size_++);
#endif
	} catch (...) {
		ed->cleanup ();
		throw;
	}
	return ed;
}

Ref <MemContext> ExecDomain::get_mem_context (SyncContext& target, Heap* heap)
{
	Ref <MemContext> mem_context;
	SyncDomain* sd = target.sync_domain ();
	if (sd) {
		assert (!heap || heap == &sd->mem_context ().heap ());
		mem_context = &sd->mem_context ();
	} else if (heap)
		mem_context = MemContext::create (*heap);

	return mem_context;
}

void ExecDomain::async_call (const DeadlineTime& deadline, Runnable& runnable,
	SyncContext& target, Heap* heap)
{
	Ref <ExecDomain> exec_domain = create (deadline, target, heap);
	exec_domain->runnable_ = &runnable;
	exec_domain->spawn (target);
}

void ExecDomain::spawn (SyncContext& sync_context)
{
	assert (ExecContext::current_ptr () != this);
	assert (runnable_);
	_add_ref ();
	try {
		Ref <SyncContext> tmp (&sync_context);
		schedule (tmp);
	} catch (...) {
		_remove_ref ();
		throw;
	}
}

void ExecDomain::execute () noexcept
{
	Thread::current ().exec_domain (*this);
	if (!impersonation_context_.empty ())
		Thread::impersonate (impersonation_context_);
	switch_to ();
}

void ExecDomain::cleanup () noexcept
{
	assert (!runnable_);

	if (sync_context_) {
		SyncDomain* sd = sync_context_->sync_domain ();
		if (sd)
			sd->leave ();
	}
	sync_context_ = nullptr;

	assert (!mem_context_stack_.empty ());
	assert (1 == dbg_mem_context_stack_size_);
	do {
		mem_context_pop ();
	} while (!mem_context_stack_.empty ());

#ifndef NDEBUG
	dbg_mem_context_stack_size_ = 0;
	dbg_suspend_prepared_ = false;
#endif

	ret_qnodes_clear ();

	if (scheduler_item_created_) {
		Scheduler::delete_item ();
		scheduler_item_created_ = false;
	}
	
	if (background_worker_) {
		background_worker_->finish ();
		background_worker_ = nullptr;
	}

	std::fill_n (tls_, CoreTLS::CORE_TLS_COUNT, nullptr);

	impersonation_context_.clear ();

	resume_exception_.reset ();

	ExecContext::run_in_neutral_context (deleter_);
}

void ExecDomain::final_release () noexcept
{
	Creator::release (*this);
	Scheduler::activity_end ();
}

void ExecDomain::Deleter::run ()
{
	ExecDomain* ed = Thread::current ().exec_domain ();
	assert (ed);
	Thread::current ().yield ();
	ed->_remove_ref ();
}

MemContext& ExecDomain::mem_context ()
{
	if (!mem_context_) {
		mem_context_ =
			mem_context_stack_.top () = MemContext::create (
				sync_context ().sync_context_type () == SyncContext::Type::FREE_MODULE_INIT);
	}
	return *mem_context_;
}

void ExecDomain::mem_context_push (Ref <MemContext>&& mem_context)
{
	// Ensure that memory context is not temporary replaced
	assert ((mem_context_ == nullptr && mem_context_stack_.empty ()) || mem_context_ == mem_context_stack_.top ());

	MemContext* p = mem_context;
	mem_context_stack_.emplace (std::move (mem_context));
	mem_context_ = p;
#ifndef NDEBUG
	++dbg_mem_context_stack_size_;
#endif
}

void ExecDomain::mem_context_pop () noexcept
{
	// Ensure that memory context is not temporary replaced
	assert (mem_context_ == mem_context_stack_.top ());

	// On releasing the memory context reference, stack must be consistent.
	Ref <MemContext> tmp (std::move (mem_context_stack_.top ()));

	mem_context_stack_.pop ();
#ifndef NDEBUG
	--dbg_mem_context_stack_size_;
#endif
	if (!mem_context_stack_.empty ())
		mem_context_ = mem_context_stack_.top ();
	else
		mem_context_ = nullptr;
}

void ExecDomain::create_background_worker ()
{
	if (!background_worker_)
		background_worker_ = ThreadBackground::spawn ();
}

void ExecDomain::schedule (Ref <SyncContext>& target, bool ret)
{
	// Called in neutral execution context
	assert (ExecContext::current_ptr () != this);

	Ref <SyncDomain> sync_domain = target->sync_domain ();
	bool background = false;
	if (!sync_domain) {
		if (INFINITE_DEADLINE == deadline ()) {
			background = true;
			assert (!ret || background_worker_);
			if (!ret)
				create_background_worker ();
		} else if (!scheduler_item_created_) {
			Scheduler::create_item ();
			scheduler_item_created_ = true;
		}
	}

	sync_context_.swap (target);
	try {
		if (sync_domain) {
			if (ret)
				sync_domain->schedule (ret_qnode_pop (), deadline (), *this);
			else
				sync_domain->schedule (deadline (), *this);
		} else {
			if (background)
				background_worker_->execute (*this);
			else
				Scheduler::schedule (deadline (), *this);
		}
	} catch (...) {
		sync_context_.swap (target);
		throw;
	}
}

void ExecDomain::schedule_call_no_push_mem (SyncContext& target)
{
	// If old context is a synchronization domain, we
	// allocate queue node to perform return without a risk
	// of memory allocation failure.
	{
		Ref <SyncDomain> old_sd = sync_context ().sync_domain ();
		if (old_sd) {
			ret_qnode_push (*old_sd);
			old_sd->leave ();
		} else if (deadline () == INFINITE_DEADLINE)
			create_background_worker (); // Prepare to return to background
	}

	if (target.sync_domain () ||
		!(deadline () == INFINITE_DEADLINE && &Thread::current () == background_worker_)) {
		// Need to schedule

		// Call schedule() in the neutral context
		schedule_.sync_context_ = &target;
		schedule_.ret_ = false;
		ExecContext::run_in_neutral_context (schedule_);
		schedule_.sync_context_ = nullptr;
		// Handle possible schedule() exceptions
		if (CORBA::SystemException::EC_NO_EXCEPTION != schedule_.exception_) {
			CORBA::SystemException::Code exc = schedule_.exception_;
			schedule_.exception_ = CORBA::SystemException::EC_NO_EXCEPTION;
			// We leaved old sync domain so we must enter into prev synchronization domain back
			// before throwing the exception.
			schedule_return (sync_context (), true);
			CORBA::SystemException::_raise_by_code (exc);
		}

	} else
		sync_context (target);
}

void ExecDomain::schedule_return (SyncContext& target, bool no_reschedule) noexcept
{
	mem_context_pop ();

	if (no_reschedule && (&sync_context () == &target))
		return;

	{
		Ref <SyncDomain> old_sd = sync_context ().sync_domain ();
		if (old_sd)
			old_sd->leave ();
	}

	if (target.sync_domain ()
		|| !(deadline () == INFINITE_DEADLINE && &Thread::current () == background_worker_)) {

		schedule_.sync_context_ = &target;
		schedule_.ret_ = true;
		ExecContext::run_in_neutral_context (schedule_);
		schedule_.sync_context_ = nullptr;
		// schedule() can not throw exception in the return mode.
	} else
		sync_context (target);
}

DeadlineTime ExecDomain::get_request_deadline (bool oneway) const noexcept
{
	const DeadlinePolicyContext* deadline_context = nullptr;
	{
		MemContext* mc = mem_context_ptr ();
		if (mc)
			deadline_context = mc->deadline_policy_ptr ();
	}

	Nirvana::System::DeadlinePolicy deadline_policy = deadline_context ?
		(oneway ? deadline_context->policy_oneway () : deadline_context->policy_async ())
		:
		(oneway ? DeadlinePolicyContext::ONEWAY_DEFAULT : DeadlinePolicyContext::ASYNC_DEFAULT);

	DeadlineTime dl;
	if (deadline_policy == Nirvana::System::DEADLINE_POLICY_INHERIT)
		dl = deadline ();
	else if (deadline_policy == Nirvana::System::DEADLINE_POLICY_INFINITE)
		dl = INFINITE_DEADLINE;
	else
		dl = Chrono::make_deadline (deadline_policy);

	return dl;
}

void ExecDomain::Schedule::run ()
{
	Thread& th = Thread::current ();
	ExecDomain* ed = th.exec_domain ();
	assert (ed);
	try {
		th.yield ();
		ed->schedule (sync_context_, ret_);
	} catch (const CORBA::SystemException& ex) {
		th.exec_domain (*ed);
		exception_ = ex.__code ();
	}
}

void ExecDomain::suspend_prepare (SyncContext* resume_context, bool push_qnode)
{
	assert (Thread::current ().exec_domain () == this);
	assert (!dbg_suspend_prepared_);
	SyncDomain* sync_domain = sync_context_->sync_domain ();
	if (sync_domain) {
		if (!resume_context || push_qnode)
			ret_qnode_push (*sync_domain);
		sync_domain->leave ();
	}
	if (resume_context)
		sync_context_ = resume_context;

#ifndef NDEBUG
	dbg_suspend_prepared_ = true;
#endif
}

void ExecDomain::suspend_prepared () noexcept
{
#ifndef NDEBUG
	assert (Thread::current ().exec_domain () == this);
	assert (&ExecContext::current () == &Thread::current ().neutral_context ());
	assert (dbg_suspend_prepared_);
	dbg_suspend_prepared_ = false;
#endif

	Thread::current ().yield ();
	resume_exception_.check ();
}

void ExecDomain::resume () noexcept
{
	assert (ExecContext::current_ptr () != this);
	assert (sync_context_);

	// Hold reference to sync_context_ to avoid it's destruction out of context.
	Ref <SyncContext> tmp (sync_context_);

	// schedule with ret = true does not throw exceptions
	schedule (tmp, true);
}

bool ExecDomain::reschedule ()
{
	Thread& thr = Thread::current ();
	ExecDomain* ed = thr.exec_domain ();
	assert (ed);
	if (&thr != ed->background_worker_) {
		ed->suspend_prepare ();
		ExecContext::run_in_neutral_context (reschedule_);
		return true;
	}
	return false;
}

void ExecDomain::Reschedule::run ()
{
	ExecDomain& ed = ExecDomain::current ();
	ed.suspend_prepared ();
	ed.resume ();
}

void ExecDomain::suspend ()
{
	assert (dbg_suspend_prepared ());
	ExecContext::run_in_neutral_context (suspend_);
}

void ExecDomain::Suspend::run ()
{
	ExecDomain::current ().suspend_prepared ();
}

bool ExecDomain::on_signal (const siginfo_t& signal)
{
	// TODO: Check for the signal handlers and return true if signal is handled.
	Binder::raise_exception (signal);

	return false;
}

}
}
