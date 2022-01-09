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

using namespace std;

namespace Nirvana {
namespace Core {

StaticallyAllocated <ExecDomain::Suspend> ExecDomain::suspend_;

CoreRef <ExecDomain> ExecDomain::create (const DeadlineTime& deadline, Runnable& runnable)
{
	return CoreRef <ExecDomain>::create <ExecDomain> (deadline, ref (runnable));
}

void ExecDomain::spawn (SyncContext& sync_context)
{
	assert (&ExecContext::current () != this);
	assert (runnable_);
	start ([this, &sync_context]() {this->schedule (sync_context); });
}

void ExecDomain::schedule (SyncContext& sync_context)
{
	assert (&ExecContext::current () != this);

	CoreRef <SyncContext> old_context = move (sync_context_);
	SyncDomain* sync_domain = sync_context.sync_domain ();
	if (!sync_domain && !scheduler_item_created_) {
		Scheduler::create_item ();
		scheduler_item_created_ = true;
	}

	sync_context_ = &sync_context;
	try {
		if (sync_domain)
			sync_domain->schedule (deadline (), *this);
		else
			Scheduler::schedule (deadline (), *this);
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

void ExecDomain::suspend ()
{
	SyncDomain* sync_domain = sync_context_->sync_domain ();
	if (sync_domain) {
		ret_qnode_push (*sync_domain);
		sync_domain->leave ();
	}
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
	runtime_support_.cleanup ();
	runtime_global_.cleanup ();
	heap_.cleanup (); // TODO: Detect and log the memory leaks.
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
			run_in_neutral_context (deleter_);
		else
			delete this;
	}
}

void ExecDomain::Deleter::run ()
{
	delete &exec_domain_;
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

void ExecDomain::schedule_call (SyncContext& sync_context)
{
	schedule_call_.sync_context_ = &sync_context;
	run_in_neutral_context (schedule_call_);
	if (schedule_call_.exception_) {
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
