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
#include "SystemInfo.h"
#include <Nirvana/bitutils.h>
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

	using Counters =
#if ATOMIC_LLONG_LOCK_FREE
		uint64_t;
#elif ATOMIC_LONG_LOCK_FREE
		uint32_t;
#else
		uintptr_t;
#endif

public:
	SchedulerImpl () noexcept :
		queue_ (SystemInfo::hardware_concurrency ()),
		counters_ (SystemInfo::hardware_concurrency ())
	{
		unsigned shift = sizeof (unsigned) * 8 - nlz ((unsigned)SystemInfo::hardware_concurrency ());
		cores_mask_ = ~(~(Counters)0 << shift);
	}

	~SchedulerImpl ()
	{
		assert (counters_ == SystemInfo::hardware_concurrency ());
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

	void schedule (const DeadlineTime& deadline, const ExecutorRef& executor) noexcept
	{
		NIRVANA_VERIFY (queue_.insert (deadline, executor));
		counters_.fetch_add (cores_mask_ + 1);
		schedule ();
	}

	bool reschedule (const DeadlineTime& deadline, const ExecutorRef& executor, const DeadlineTime& deadline_prev) noexcept
	{
		return queue_.reorder (deadline, executor, deadline_prev);
	}

	void core_free () noexcept;

private:
	void schedule () noexcept;

private:
	Queue queue_;
	std::atomic <Counters> counters_;
	Counters cores_mask_;
};

template <class T, class ExecutorRef>
void SchedulerImpl <T, ExecutorRef>::core_free () noexcept
{
	Counters cnt = counters_.load (std::memory_order_acquire);
	for (;;) {
		Counters cores = (cnt & cores_mask_) + 1;
		if (cores > SystemInfo::hardware_concurrency ()) {
			assert (false);
			// TODO: Log
			return;
		}
		Counters cnt_new = (cnt & ~cores_mask_) | cores;
		if (counters_.compare_exchange_weak (cnt, cnt_new))
			break;
	}
	schedule ();
}

template <class T, class ExecutorRef>
void SchedulerImpl <T, ExecutorRef>::schedule () noexcept
{
	Counters cnt = counters_.load (std::memory_order_acquire);
	for (;;) {
		// We must have at least one item in queue and at least one free processor core
		if (!(cnt & cores_mask_) || !(cnt & ~cores_mask_))
			return;

		// Decrement both counters as a whole
		Counters cnt_new = cnt - (cores_mask_ + 2);
		if (counters_.compare_exchange_weak (cnt, cnt_new))
			break;
	}

	ExecutorRef val;
	NIRVANA_VERIFY (queue_.delete_min (val));
	if (!static_cast <T*> (this)->execute (std::move (val))) {
		// Error in protection domain execution, probably it is terminated.
		core_free ();
	}
}

}
}

#endif
