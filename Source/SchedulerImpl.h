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

#include "PriorityQueue.h"
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
	typedef SkipListWithPool <PriorityQueue <ExecutorRef, SYS_DOMAIN_PRIORITY_QUEUE_LEVELS> > Queue;
public:
	SchedulerImpl () NIRVANA_NOEXCEPT :
		free_cores_ (Port::SystemInfo::hardware_concurrency ()),
		queue_ (Port::SystemInfo::hardware_concurrency ())
	{}

	void create_item ()
	{
		queue_.create_item ();
	}

	void delete_item () NIRVANA_NOEXCEPT
	{
		queue_.delete_item ();
	}

	void schedule (const DeadlineTime& deadline, const ExecutorRef& executor) NIRVANA_NOEXCEPT
	{
		verify (queue_.insert (deadline, executor));
		execute_next ();
	}

	bool reschedule (const DeadlineTime& deadline, const ExecutorRef& executor, const DeadlineTime& deadline_prev) NIRVANA_NOEXCEPT
	{
		return queue_.reorder (deadline, executor, deadline_prev);
	}

	void core_free () NIRVANA_NOEXCEPT
	{
		free_cores_.increment ();
		if (!queue_.empty ())
			execute_next ();
	}

private:
	void execute_next () NIRVANA_NOEXCEPT;

protected:
	AtomicCounter <true> free_cores_;

private:
	Queue queue_;
};

template <class T, class ExecutorRef>
void SchedulerImpl <T, ExecutorRef>::execute_next () NIRVANA_NOEXCEPT
{
	do {

		// Acquire processor core
		for (;;) {
			if (free_cores_.decrement_seq () >= 0)
				break;
			if (free_cores_.increment_seq () <= 0)
				return;
		}

		// Get first item
		ExecutorRef val;
		if (queue_.delete_min (val)) {
			static_cast <T*> (this)->execute (val);
			break;
		}

		// Release processor core
		free_cores_.increment ();

	} while (!queue_.empty ());
}

}
}

#endif
