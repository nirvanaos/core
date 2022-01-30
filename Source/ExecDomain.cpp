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
	Scheduler::activity_begin ();	// Throws exception if shutdown was started.
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

void ExecDomain::start_legacy_thread (Runnable& runnable, MemContext& mem_context)
{
	CoreRef <ExecDomain> exec_domain = create (INFINITE_DEADLINE, runnable, &mem_context);
	exec_domain->background_worker_ = Legacy::Core::ThreadLegacy::create (*exec_domain);
	exec_domain->spawn (g_core_free_sync_context);
}

void ExecDomain::schedule (SyncContext& sync_context)
{
	assert (&ExecContext::current () != this);

	CoreRef <SyncContext> old_context = move (sync_context_);
	SyncDomain* sync_domain = sync_context.sync_domain ();
	bool background = false;
	if (!sync_domain) {
		if (INFINITE_DEADLINE == deadline ())
			background = true;
		else if (!scheduler_item_created_) {
			Scheduler::create_item ();
			scheduler_item_created_ = true;
		}
	}

	sync_context_ = &sync_context;
	try {
		if (sync_domain)
			sync_domain->schedule (deadline (), *this);
		else {
			if (background) {
				if (!background_worker_)
					background_worker_ = ThreadBackground::create (*this);
				background_worker_->resume ();
			} else
				Scheduler::schedule (deadline (), *this);
		}
	} catch (...) {
		sync_context_ = move (old_context);
		throw;
	}

	if (old_context) {
		SyncDomain* old_sd = old_context->sync_domain ();
		if (old_sd)
			old_sd->leave ();
	}
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
		sync_context (*resume_context);
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
	_remove_ref ();
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

void ExecDomain::run () NIRVANA_NOEXCEPT
{
	assert (Thread::current ().exec_domain () == this);
	assert (runnable_);
	if (scheduler_error_ >= 0) {
		runnable_->on_crash (scheduler_error_);
		runnable_ = nullptr;
	} else {
		ExecContext::run ();
		assert (!runnable_); // Cleaned inside ExecContext::run ();
	}
	cleanup ();
}

void ExecDomain::on_exec_domain_crash (CORBA::SystemException::Code err) NIRVANA_NOEXCEPT
{
	ExecContext::on_crash (err);
	cleanup ();
}

void ExecDomain::schedule_call (SyncContext& sync_context, MemContext* mem_context)
{
	SyncDomain* sd = sync_context.sync_domain ();
	if (sd) {
		assert (!mem_context || mem_context == &sd->mem_context ());
		mem_context = &sd->mem_context ();
	}
	mem_context_push (mem_context);
	schedule_call_.sync_context_ = &sync_context;
	run_in_neutral_context (schedule_call_);
	if (schedule_call_.exception_) {
		mem_context_pop ();
		exception_ptr ex = schedule_call_.exception_;
		schedule_call_.exception_ = nullptr;
		rethrow_exception (ex);
	}
}

void ExecDomain::ScheduleCall::run ()
{
	exec_domain_.schedule (*sync_context_);
	Thread::current ().yield ();
}

void ExecDomain::ScheduleCall::on_exception () NIRVANA_NOEXCEPT
{
	exception_ = current_exception ();
}

void ExecDomain::schedule_return (SyncContext& sync_context) NIRVANA_NOEXCEPT
{
	mem_context_pop ();
	schedule_return_.sync_context_ = &sync_context;
	run_in_neutral_context (schedule_return_);
}

void ExecDomain::ScheduleReturn::run ()
{
	Thread& thread = Thread::current ();
	CoreRef <SyncDomain> old_sync_domain = exec_domain_.sync_context ().sync_domain ();
	sync_context_->schedule_return (exec_domain_);
	if (old_sync_domain)
		old_sync_domain->leave ();
	thread.yield ();
}

}
}
