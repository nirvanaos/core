/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include "AtomicCounter.h"
#include "SystemInfo.h"
#include "unrecoverable_error.h"
#include <atomic>

namespace Nirvana {
namespace Core {

/// Template for implementing the master scheduler.
/// \tparam T Derived class implementing the scheduler.
///           T must implement public method void execute (const ExecutorRef&);
/// \tparam ExecutorRef Executor reference type.
template <class T, class ExecutorRef>
class SchedulerImpl
{
	using Queue = SkipListWithPool <PriorityQueueReorder <ExecutorRef, SKIP_LIST_DEFAULT_LEVELS> >;

public:
	SchedulerImpl () noexcept :
		queue_ (SystemInfo::hardware_concurrency ()),
		queue_items_ (0),
		free_cores_ (SystemInfo::hardware_concurrency ())
	{}

	~SchedulerImpl ()
	{
		assert (queue_items_ == 0 && free_cores_ == SystemInfo::hardware_concurrency ());
	}

	void create_item (bool with_reschedule)
	{
		queue_.create_item ();
		if (with_reschedule)
			queue_.create_item ();
	}

	void delete_item (bool with_reschedule) noexcept
	{
		queue_.delete_item ();
		if (with_reschedule)
			queue_.delete_item ();
	}

	bool reschedule (const DeadlineTime& deadline, const ExecutorRef& executor, const DeadlineTime& deadline_prev)
	{
		return queue_.reorder (deadline, executor, deadline_prev);
	}

	void schedule (const DeadlineTime& deadline, const ExecutorRef& executor)
	{
		NIRVANA_VERIFY (queue_.insert (deadline, executor));

		for (;;) {
			if (free_cores_.decrement_if_not_zero ()) {
				if (execute ())
					return;
				free_cores_.increment ();
			}
			if (queue_items_.increment_seq () == 1) {
				if (!free_cores_.load ())
					break;
				if (!queue_items_.decrement_if_not_zero ())
					break;
			} else
				break;
		}
	}

	void core_free () noexcept
	{
		for (;;) {
			while (queue_items_.decrement_if_not_zero ()) {
				if (execute ())
					return;
			}
			auto cores = free_cores_.increment_seq ();
			assert (cores <= SystemInfo::hardware_concurrency ());
			if (cores == 1) {
				if (!queue_items_.load ())
					break;
				if (!free_cores_.decrement_if_not_zero ())
					break;
			} else
				break;
		}
	}

private:
	bool execute () noexcept;

private:
	Queue queue_;
	AtomicCounter <false> queue_items_;
	AtomicCounter <false> free_cores_;
};

template <class T, class ExecutorRef>
bool SchedulerImpl <T, ExecutorRef>::execute () noexcept
{
	ExecutorRef val;
	NIRVANA_VERIFY (queue_.delete_min (val));
	return static_cast <T*> (this)->execute (std::move (val));
}

}
}

#endif
