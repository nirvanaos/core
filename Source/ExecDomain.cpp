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
#include "Thread.inl"

namespace Nirvana {
namespace Core {

ObjectPool <ExecDomain> ExecDomain::pool_;

Core_var <ExecDomain> ExecDomain::get (DeadlineTime deadline)
{
	Scheduler::activity_begin ();	// Throws exception if shutdown was started.
	try {
		Core_var <ExecDomain> exec_domain = pool_.get ();
		exec_domain->deadline_ = deadline;
		return exec_domain;
	} catch (...) {
		Scheduler::activity_end ();
		throw;
	}
}

void ExecDomain::ctor_base ()
{
	wait_list_next_ = nullptr;
	deadline_ = std::numeric_limits <DeadlineTime>::max ();
	ret_qnodes_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
	scheduler_item_created_ = false;
	stateless_creation_frame_ = nullptr;
	cur_heap_ = &heap_;
	release_to_pool_.init (*this);
	schedule_call_.init (*this);
	schedule_return_.init (*this);
}

void ExecDomain::spawn (SyncDomain* sync_domain)
{
	assert (&ExecContext::current () != this);
	assert (runnable_);
	start ([this, sync_domain]() {this->schedule (sync_domain); });
}

void ExecDomain::schedule (SyncDomain* sync_domain)
{
	assert (&ExecContext::current () != this);

	Core_var <SyncContext> old_context = std::move (sync_context_);
	if (!sync_domain && !scheduler_item_created_) {
		Scheduler::create_item ();
		scheduler_item_created_ = true;
	}

	try {
		if (sync_domain) {
			sync_context_ = sync_domain;
			sync_domain->schedule (deadline (), *this);
		} else {
			sync_context_ = &SyncContext::free_sync_context ();
			Scheduler::schedule (deadline (), *this);
		}
	} catch (...) {
		sync_context_ = std::move (old_context);
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
	ExecDomain* cur = Thread::current ().exec_domain ();
	assert (cur);
	cur->sync_context ()->schedule_call (SyncContext::SUSPEND ());
}

void ExecDomain::resume ()
{
	assert (&ExecContext::current () != this);
	assert (sync_context_);
	sync_context_->schedule_return (*this);
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
	heap_.cleanup (); // TODO: Detect and log the memory leaks.
	sync_context_ = nullptr;
	scheduler_error_ = CORBA::SystemException::EC_NO_EXCEPTION;
	ret_qnodes_clear ();
	if (scheduler_item_created_) {
		Scheduler::delete_item ();
		scheduler_item_created_ = false;
	}
}

void ExecDomain::execute_loop () NIRVANA_NOEXCEPT
{
	assert (Thread::current ().exec_domain () == this);
	while (runnable_) {
		if (scheduler_error_ >= 0) {
			runnable_->on_crash (scheduler_error_);
			runnable_ = nullptr;
		} else {
			ExecContext::run ();
			assert (!runnable_); // Cleaned inside ExecContext::run ();
		}
		cleanup ();
		Thread::current ().exec_domain (nullptr);
		_remove_ref ();
	}
}

void ExecDomain::on_exec_domain_crash (CORBA::SystemException::Code err) NIRVANA_NOEXCEPT
{
	Thread::current ().exec_domain (nullptr);
	ExecContext::on_crash (err);
	cleanup ();
	_remove_ref ();
}

void ExecDomain::ReleaseToPool::run ()
{
	assert (&ExecContext::current () != exec_domain_);
	pool_.release (static_cast <ImplPoolable <ExecDomain>&> (*exec_domain_));
}

void ExecDomain::schedule_call (SyncDomain* sync_domain)
{
	schedule_call_.sync_domain_ = sync_domain;
	run_in_neutral_context (schedule_call_);
	if (schedule_call_.exception_) {
		std::exception_ptr ex = schedule_call_.exception_;
		schedule_call_.exception_ = nullptr;
		rethrow_exception (ex);
	}
}

void ExecDomain::ScheduleCall::run ()
{
	exec_domain_->schedule (sync_domain_);
	Thread::current ().yield ();
}

void ExecDomain::ScheduleCall::on_exception () NIRVANA_NOEXCEPT
{
	exception_ = std::current_exception ();
}

void ExecDomain::schedule_return (SyncContext& sync_context) NIRVANA_NOEXCEPT
{
	schedule_return_.sync_context_ = &sync_context;
	run_in_neutral_context (schedule_return_);
}

void ExecDomain::ScheduleReturn::run ()
{
	Thread& thread = Thread::current ();
	Core_var <SyncDomain> old_sync_domain = exec_domain_->sync_context ()->sync_domain ();
	sync_context_->schedule_return (*exec_domain_);
	if (old_sync_domain)
		old_sync_domain->leave ();
	thread.yield ();
}

}
}
