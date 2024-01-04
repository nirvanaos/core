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
#include "Legacy/Process.h"
#include "Chrono.h"
#include "MemContextCore.h"
#include "MemContextImpl.h"
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
				ed->security_context_ = parent->security_context_;
		}

#ifndef NDEBUG
		assert (!ed->dbg_context_stack_size_++);
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
		mem_context = MemContextCore::create (heap);

	return mem_context;
}

void ExecDomain::async_call (const DeadlineTime& deadline, Runnable* runnable,
	SyncContext& target, Heap* heap)
{
	assert (runnable);
	Ref <ExecDomain> exec_domain = create (deadline, target, heap);
	exec_domain->runnable_ = runnable;
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

void ExecDomain::start_legacy_thread (Legacy::Core::Process& process, Legacy::Core::ThreadBase& thread)
{
	// The process must inherit the current heap.
	assert (&process.heap () == &current ().mem_context ().heap ());

	Ref <ExecDomain> exec_domain = create (INFINITE_DEADLINE, &process);
	exec_domain->runnable_ = &thread;
	exec_domain->background_worker_ = &thread;
	exec_domain->spawn (process.sync_context ());
}

void ExecDomain::execute () noexcept
{
	Thread::current ().exec_domain (*this);
	if (!security_context_.empty ())
		Thread::impersonate (security_context_);
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

	ret_qnodes_clear ();
	
	assert (!mem_context_stack_.empty ());
	for (;;) {
		mem_context_stack_.pop ();
		if (mem_context_stack_.empty ())
			break;
		mem_context_ = mem_context_stack_.top ();
	}
	mem_context_ = nullptr;

#ifndef NDEBUG
	dbg_context_stack_size_ = 0;
#endif

	if (scheduler_item_created_) {
		Scheduler::delete_item ();
		scheduler_item_created_ = false;
	}
	
	if (background_worker_) {
		background_worker_->finish ();
		background_worker_ = nullptr;
	}

	std::fill_n (tls_, CoreTLS::CORE_TLS_COUNT, nullptr);

	security_context_.clear ();

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
			mem_context_stack_.top () = MemContextImpl::create ();
	}
	return *mem_context_;
}

void ExecDomain::mem_context_push (Ref <MemContext>&& mem_context)
{
	MemContext* p = mem_context;
	mem_context_stack_.emplace (std::move (mem_context));
	mem_context_ = p;
#ifndef NDEBUG
	++dbg_context_stack_size_;
#endif
}

void ExecDomain::mem_context_pop () noexcept
{
	mem_context_stack_.pop ();
#ifndef NDEBUG
	--dbg_context_stack_size_;
#endif
	mem_context_ = mem_context_stack_.top ();
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

	SyncDomain* sync_domain = target->sync_domain ();
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
	SyncDomain* old_sd = sync_context ().sync_domain ();
	if (old_sd) {
		ret_qnode_push (*old_sd);
		old_sd->leave ();
	} else if (deadline () == INFINITE_DEADLINE)
		create_background_worker (); // Prepare to return to background

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

	SyncDomain* old_sd = sync_context ().sync_domain ();
	if (old_sd)
		old_sd->leave ();

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
	const DeadlineTime dp = oneway ?
		(mem_context_ptr () ? mem_context_ptr ()->deadline_policy_oneway () : INFINITE_DEADLINE)
		:
		(mem_context_ptr () ? mem_context_ptr ()->deadline_policy_async () : 0);
	DeadlineTime dl = INFINITE_DEADLINE;
	if (dp == 0)
		dl = deadline ();
	else if (INFINITE_DEADLINE == dp)
		dl = INFINITE_DEADLINE;
	else
		dl = Chrono::make_deadline (dp);
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

void ExecDomain::suspend_prepare (SyncContext* resume_context)
{
	assert (Thread::current ().exec_domain () == this);
	SyncDomain* sync_domain = sync_context_->sync_domain ();
	if (sync_domain) {
		if (!resume_context)
			ret_qnode_push (*sync_domain);
		sync_domain->leave ();
	}
	if (resume_context)
		sync_context_ = resume_context;
}

bool ExecDomain::reschedule ()
{
	Thread& thr = Thread::current ();
	ExecDomain* ed = thr.exec_domain ();
	assert (ed);
	if (&thr != ed->background_worker_) {
		ExecContext::run_in_neutral_context (reschedule_);
		return true;
	}
	return false;
}

void ExecDomain::Reschedule::run ()
{
	ExecDomain* ed = Thread::current ().exec_domain ();
	assert (ed);
	try {
		ed->suspend_prepare (nullptr);
		ed->suspend_prepared ();
	} catch (...) {
		return;
	}
	ed->resume ();
}

void ExecDomain::suspend (SyncContext* resume_context)
{
	suspend_.resume_context_ = resume_context;
	ExecContext::run_in_neutral_context (suspend_);
	suspend_.resume_context_ = nullptr;
	// Handle possible exceptions
	if (CORBA::SystemException::EC_NO_EXCEPTION != suspend_.exception_) {
		CORBA::SystemException::Code exc = suspend_.exception_;
		suspend_.exception_ = CORBA::SystemException::EC_NO_EXCEPTION;
		CORBA::SystemException::_raise_by_code (exc);
	}
}

void ExecDomain::Suspend::run ()
{
	ExecDomain* ed = Thread::current ().exec_domain ();
	assert (ed);
	try {
		ed->suspend_prepare (resume_context_);
		ed->suspend_prepared ();
	} catch (const CORBA::SystemException& ex) {
		exception_ = ex.__code ();
	}
}

}
}
