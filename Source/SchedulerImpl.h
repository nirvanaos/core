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
#ifndef NIRVANA_CORE_SCHEDULERIMPL_H_
#define NIRVANA_CORE_SCHEDULERIMPL_H_
#pragma once

#include "PriorityQueueReorder.h"
#include "SkipListWithPool.h"
#include <Port/SystemInfo.h>

namespace Nirvana {
namespace Core {

/// Template for implementing the master scheduler.
/// \tparam T Derived class implementing the scheduler.
///           T must implement public method void execute (const ExecutorRef&);
/// \tparam ExecutorRef Executor reference type.
template <class T, class ExecutorRef>
class SchedulerImpl
{
	typedef SkipListWithPool <PriorityQueueReorder <ExecutorRef, SYS_DOMAIN_PRIORITY_QUEUE_LEVELS> > Queue;
public:
	SchedulerImpl () noexcept :
		queue_ (Port::SystemInfo::hardware_concurrency ()),
		free_cores_ (Port::SystemInfo::hardware_concurrency ()),
		queue_items_ (0)
	{}

	~SchedulerImpl ()
	{
		assert (free_cores_ == Port::SystemInfo::hardware_concurrency ());
		assert (queue_items_ == 0);
	}

	void create_item ()
	{
		queue_.create_item ();
	}

	void delete_item () noexcept
	{
		queue_.delete_item ();
	}

	void schedule (const DeadlineTime& deadline, const ExecutorRef& executor) noexcept
	{
		NIRVANA_VERIFY (queue_.insert (deadline, executor));
		queue_items_.increment ();
		execute_next ();
	}

	bool reschedule (const DeadlineTime& deadline, const ExecutorRef& executor, const DeadlineTime& deadline_prev) noexcept
	{
		if (queue_.reorder (deadline, executor, deadline_prev)) {
			execute_next ();
			return true;
		}
		return false;
	}

	void core_free () noexcept
	{
		if (execute ())
			return;
		free_cores_.increment ();
		if (queue_items_.load () > 0)
			execute_next ();
	}

private:
	bool execute () noexcept;
	void execute_next () noexcept;

private:
	Queue queue_;
	AtomicCounter <false> free_cores_;
	AtomicCounter <true> queue_items_;
};

template <class T, class ExecutorRef>
bool SchedulerImpl <T, ExecutorRef>::execute () noexcept
{
	// Get first item
	ExecutorRef val;
	while (queue_.delete_min (val)) {
		queue_items_.decrement ();
		if (static_cast <T*> (this)->execute (val))
			return true;
	}
	return false;
}

template <class T, class ExecutorRef>
void SchedulerImpl <T, ExecutorRef>::execute_next () noexcept
{
	do {
		// Acquire processor core
		if (!free_cores_.decrement_if_not_zero ())
			return; // All cores are busy

		// We successfully acquired the processor core

		// Get item from queue
		if (execute ())
			break;

		// Queue is empty, release processor core
		free_cores_.increment ();

		// Other thread may add item to the queue but fail to acquire the core,
		// because this thread was holded it. So we must retry if the queue is not empty.
	} while (queue_items_.load () > 0);
}

}
}

#endif
