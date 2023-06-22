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
#include "ExecDomain.h"
#include "Legacy/Process.h"
#include "Chrono.h"
#include "MemContextUser.h"
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

	static void release (ExecDomain& ed)
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

	static void release (ExecDomain& ed)
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
	ed->deadline_ = deadline;
	assert (ed->mem_context_stack_.empty ());

	MemContext* pmc = mem_context;
	ed->mem_context_stack_.push (std::move (mem_context));
	ed->mem_context_ = pmc;
#ifdef _DEBUG
	assert (!ed->dbg_context_stack_size_++);
#endif
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
		mem_context = Ref <MemContext>::create <MemContextCore> (std::ref (*heap));

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

void ExecDomain::start_legacy_process (Legacy::Core::Process& process)
{
	// The process must inherit the current heap.
	assert (&process.heap () == &current ().mem_context ().heap ());

	// Start thread with process as the memory context.
	start_legacy_thread (process, process);
}

void ExecDomain::start_legacy_thread (Legacy::Core::Process& process, Legacy::Core::ThreadBase& thread)
{
	Ref <ExecDomain> exec_domain = create (INFINITE_DEADLINE, &process);
	exec_domain->runnable_ = &thread;
	exec_domain->background_worker_ = &thread;
	exec_domain->spawn (process.sync_context ());
}

void ExecDomain::execute ()
{
	Thread::current ().exec_domain (*this);
	ExecContext::switch_to ();
}

void ExecDomain::cleanup () noexcept
{
	assert (this == &current ());
	assert (!runnable_);

	{
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

#ifdef _DEBUG
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

	runtime_global_.cleanup ();
	tls_.clear ();

	deadline_policy_async_._default ();
	deadline_policy_oneway_._d (System::DeadlinePolicyType::DEADLINE_INFINITE);

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
			mem_context_stack_.top () = MemContextUser::create ();
	}
	return *mem_context_;
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
			if (background) {
				assert (Thread::current_ptr () != background_worker_);
				background_worker_->execute (*this);
			} else
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
	const System::DeadlinePolicy& dp = oneway ? deadline_policy_oneway_ : deadline_policy_async_;
	DeadlineTime dl = INFINITE_DEADLINE;
	switch (dp._d ()) {
		case System::DeadlinePolicyType::DEADLINE_INHERIT:
			dl = deadline ();
			break;
		case System::DeadlinePolicyType::DEADLINE_TIMEOUT:
			dl = Chrono::make_deadline (dp.timeout ());
			break;
	}
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

void ExecDomain::suspend_prepared () noexcept
{
	assert (Thread::current ().exec_domain () == this);
	ExecContext& neutral_context = Thread::current ().neutral_context ();
	Thread::current ().yield ();
	if (&neutral_context != &ExecContext::current ())
		neutral_context.switch_to ();
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
		ed->suspend (nullptr);
	} catch (...) {
		return;
	}
	ed->resume ();
}

}
}
