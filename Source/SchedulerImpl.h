#ifndef NIRVANA_CORE_SCHEDULERIMPL_H_
#define NIRVANA_CORE_SCHEDULERIMPL_H_

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
		queue_ (Port::SystemInfo::hardware_concurrency ()),
		free_cores_ (Port::SystemInfo::hardware_concurrency ()),
		active_items_ (0)
	{}

	void create_item ()
	{
		queue_.create_item ();
		active_items_.increment ();
	}

	void delete_item () NIRVANA_NOEXCEPT
	{
		queue_.delete_item ();
		active_items_.decrement ();
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
		execute_next ();
	}

	AtomicCounter <false>::IntegralType active_items () const
	{
		return active_items_;
	}

private:
	void execute_next () NIRVANA_NOEXCEPT;

private:
	Queue queue_;
	AtomicCounter <true> free_cores_;
	AtomicCounter <false> active_items_;
};

template <class T, class ExecutorRef>
void SchedulerImpl <T, ExecutorRef>::execute_next () NIRVANA_NOEXCEPT
{
	ExecutorRef val;
	do {

		// Acquire processor core
		for (;;) {
			if (free_cores_.decrement () >= 0)
				break;
			if (free_cores_.increment () <= 0)
				return;
		}

		// Get first item
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
