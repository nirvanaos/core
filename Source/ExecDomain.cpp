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
#include "Legacy/ThreadLegacy.h"
#include <Port/SystemInfo.h>
#include <signal.h>

using namespace std;

namespace Nirvana {
namespace Core {

class ExecDomain::WithPool
{
public:
	static void initialize () NIRVANA_NOEXCEPT
	{
		// Set initial pool size with processor count.
		pool_.construct (Port::SystemInfo::hardware_concurrency ());
	}

	static void terminate () NIRVANA_NOEXCEPT
	{
		pool_.destruct ();
	}

	static CoreRef <ExecDomain> create ()
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
	static void initialize () NIRVANA_NOEXCEPT
	{}

	static void terminate () NIRVANA_NOEXCEPT
	{}

	static CoreRef <ExecDomain> create ()
	{
		return CoreRef <ExecDomain>::create <ExecDomain> ();
	}

	static void release (ExecDomain& ed)
	{
		delete& ed;
	}
};

StaticallyAllocated <ExecDomain::Suspend> ExecDomain::suspend_;

void ExecDomain::initialize ()
{
	suspend_.construct ();
	Creator::initialize ();
}

void ExecDomain::terminate () NIRVANA_NOEXCEPT
{
	Creator::terminate ();
}

inline
void ExecDomain::final_construct (const DeadlineTime& deadline, Runnable& runnable, MemContext* mem_context)
{
	Scheduler::activity_begin ();
	deadline_ = deadline;
	assert (mem_context_.empty ());
	mem_context_.push (mem_context);
#ifdef _DEBUG
	assert (!dbg_context_stack_size_++);
#endif
	runnable_ = &runnable;
}

CoreRef <ExecDomain> ExecDomain::create (const DeadlineTime deadline, Runnable& runnable, MemContext* memory)
{
	CoreRef <ExecDomain> ed = Creator::create ();
	ed->final_construct (deadline, runnable, memory);
	return ed;
}

void ExecDomain::final_release () NIRVANA_NOEXCEPT
{
	assert (!runnable_);
	assert (!sync_context_);
#ifdef _DEBUG
	Thread& thread = Thread::current ();
	assert (thread.exec_domain () != this);
#endif
	Scheduler::activity_end ();
	Creator::release (*this);
}

void ExecDomain::_add_ref () NIRVANA_NOEXCEPT
{
	ref_cnt_.increment ();
}

void ExecDomain::_remove_ref () NIRVANA_NOEXCEPT
{
	if (!ref_cnt_.decrement ()) {
		if (&ExecContext::current () == this)
			run_in_neutral_context (*deleter_);
		else
			final_release ();
	}
}

void ExecDomain::Deleter::run ()
{
	exec_domain_.final_release ();
}

void ExecDomain::spawn (SyncContext& sync_context)
{
	assert (&ExecContext::current () != this);
	assert (runnable_);
	_add_ref ();
	try {
		schedule (sync_context);
	} catch (...) {
		_remove_ref ();
		throw;
	}
}

void ExecDomain::async_call (const DeadlineTime& deadline, Runnable& runnable, SyncContext& target, MemContext* mem_context)
{
	SyncDomain* sd = target.sync_domain ();
	if (sd) {
		assert (!mem_context || mem_context == &sd->mem_context ());
		mem_context = &sd->mem_context ();
	}
	CoreRef <ExecDomain> exec_domain = create (deadline, runnable, mem_context);
	exec_domain->spawn (target);
}

void ExecDomain::start_legacy_thread (Runnable& runnable, MemContext& mem_context)
{
	CoreRef <ExecDomain> exec_domain = create (INFINITE_DEADLINE, runnable, &mem_context);
	exec_domain->background_worker_ = Legacy::Core::ThreadLegacy::create (*exec_domain);
	exec_domain->spawn (g_core_free_sync_context);
}

void ExecDomain::execute (int scheduler_error)
{
	Thread::current ().exec_domain (*this);
	scheduler_error_ = (CORBA::Exception::Code)scheduler_error;
	ExecContext::switch_to ();
}

void ExecDomain::cleanup () NIRVANA_NOEXCEPT
{
	{
		SyncDomain* sd = sync_context_->sync_domain ();
		if (sd)
			sd->leave ();
	}
	runtime_global_.cleanup ();
	mem_context_.clear (); // TODO: Detect and log the memory leaks (if domain was not crashed).
#ifdef _DEBUG
	dbg_context_stack_size_ = 0;
#endif
	stateless_creation_frame_ = nullptr;
	binder_context_ = nullptr;
	sync_context_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
	ret_qnodes_clear ();
	if (scheduler_item_created_) {
		Scheduler::delete_item ();
		scheduler_item_created_ = false;
	}
	Thread::current ().exec_domain (nullptr);
	if (background_worker_) {
		background_worker_->exec_domain (nullptr);
		background_worker_.reset ();
	}
	_remove_ref ();
}

void ExecDomain::run () NIRVANA_NOEXCEPT
{
	assert (Thread::current ().exec_domain () == this);
	assert (runnable_);
	if (scheduler_error_ >= 0) {
		try {
			CORBA::SystemException::_raise_by_code (scheduler_error_);
		} catch (...) {
			runnable_->on_exception ();
		}
		runnable_ = nullptr;
	} else {
		ExecContext::run ();
		assert (!runnable_); // Cleaned inside ExecContext::run ();
	}
	cleanup ();
}

void ExecDomain::on_crash (int signal) NIRVANA_NOEXCEPT
{
	// Leave sync domain if one.
	SyncDomain* sd = sync_context_->sync_domain ();
	if (sd)
		sd->leave ();
	sync_context_ = &g_core_free_sync_context;

	// Clear memory context stack
	CoreRef <MemContext> tmp;
	do {
		tmp = mem_context_.top ();
		mem_context_.pop ();
	} while (!mem_context_.empty ());
	mem_context_.emplace (move (tmp));

	ExecContext::on_crash (signal);
	
	cleanup ();
}

void ExecDomain::schedule (SyncContext& target, bool ret)
{
	assert (ExecContext::current_ptr () != this);

	CoreRef <SyncContext> old_context = move (sync_context_);
	SyncDomain* sync_domain = target.sync_domain ();
	bool background = false;
	if (!sync_domain) {
		if (INFINITE_DEADLINE == deadline ()) {
			if (!background_worker_)
				background_worker_ = ThreadBackground::create (*this);
			background = true;
		} else if (!scheduler_item_created_) {
			Scheduler::create_item ();
			scheduler_item_created_ = true;
		}
	}

	sync_context_ = &target;
	try {
		if (sync_domain) {
			if (ret)
				sync_domain->schedule (ret_qnode_pop (), deadline (), *this);
			else
				sync_domain->schedule (deadline (), *this);
		} else {
			if (background)
				background_worker_->resume ();
			else
				Scheduler::schedule (deadline (), *this);
		}
	} catch (...) {
		sync_context_ = move (old_context);
		throw;
	}
}

void ExecDomain::schedule_call (SyncContext& target, MemContext* mem_context)
{
	SyncDomain* sd = target.sync_domain ();
	if (sd) {
		assert (!mem_context || mem_context == &sd->mem_context ());
		mem_context = &sd->mem_context ();
	}
	mem_context_push (mem_context);
	
	if (sd || !(deadline () == INFINITE_DEADLINE && &Thread::current () == background_worker_)) {
		// Need to schedule
		SyncContext& old_context = sync_context ();

		// If old context is a synchronization domain, we
		// allocate queue node to perform return without a risk
		// of memory allocation failure.
		SyncDomain* old_sd = old_context.sync_domain ();
		if (old_sd) {
			old_sd->leave ();
			ret_qnode_push (*old_sd);
		}

		// Call schedule() in the neutral context
		schedule_.sync_context_ = &target;
		schedule_.ret_ = false;
		run_in_neutral_context (schedule_);

		// Handle possible schedule() exceptions
		if (schedule_.exception_) {
			exception_ptr ex = schedule_.exception_;
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
	}
}

void ExecDomain::schedule_return (SyncContext& target) NIRVANA_NOEXCEPT
{
	mem_context_pop ();
	if (target.sync_domain () || !(deadline () == INFINITE_DEADLINE && &Thread::current () == background_worker_)) {
		SyncDomain* old_sd = sync_context ().sync_domain ();
		if (old_sd)
			old_sd->leave ();

		schedule_.sync_context_ = &target;
		schedule_.ret_ = true;
		run_in_neutral_context (schedule_);
		// schedule() can not throw exception in the return mode.
		// Now ED in the scheduler queue.
		// Now ED is again executed by the scheduler.
		// But maybe a schedule error occurs.
		CORBA::Exception::Code err = scheduler_error ();
		if (err >= 0)
			CORBA::SystemException::_raise_by_code (err);
	}
}

void ExecDomain::Schedule::run ()
{
	exec_domain_.schedule (*sync_context_, ret_);
	Thread::current ().yield ();
}

void ExecDomain::Schedule::on_exception () NIRVANA_NOEXCEPT
{
	assert (!ret_);
	exception_ = current_exception ();
}

void ExecDomain::suspend (SyncContext* resume_context)
{
	SyncDomain* sync_domain = sync_context_->sync_domain ();
	if (sync_domain) {
		if (!resume_context)
			ret_qnode_push (*sync_domain);
		sync_domain->leave ();
	}
	if (resume_context)
		sync_context_ = resume_context;
	ExecContext& neutral_context = Thread::current ().neutral_context ();
	if (&neutral_context != &ExecContext::current ())
		neutral_context.run_in_context (suspend_);
	else
		Thread::current ().yield ();
}

void ExecDomain::Suspend::run ()
{
	Thread::current ().yield ();
}

bool ExecDomain::yield ()
{
	Thread& thr = Thread::current ();
	ExecDomain* ed = thr.exec_domain ();
	assert (ed);
	if (&thr != ed->background_worker_) {
		run_in_neutral_context (ed->yield_);
		return true;
	}
	return false;
}

void ExecDomain::Yield::run ()
{
	exec_domain_.suspend (nullptr);
	exec_domain_.resume ();
}

}
}
