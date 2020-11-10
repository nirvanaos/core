#ifndef NIRVANA_CORE_SCHEDULERIMPL_H_
#define NIRVANA_CORE_SCHEDULERIMPL_H_

#include "PriorityQueue.h"
#include "AtomicCounter.h"
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
	typedef PriorityQueue <ExecutorRef, SYS_DOMAIN_PRIORITY_QUEUE_LEVELS> Queue;
public:
	typedef Queue::NodeVal* Item;

	SchedulerImpl () NIRVANA_NOEXCEPT :
		free_cores_ (Port::g_system_info.hardware_concurrency ())
	{}

	Item create_item (const ExecutorRef& executor)
	{
		return queue_.create_node (0, executor);
	}

	void release_item (Item item) NIRVANA_NOEXCEPT
	{
		queue_.release_node (item);
	}

	void schedule (const DeadlineTime& deadline, Item item) NIRVANA_NOEXCEPT
	{
		verify (queue_.insert (deadline, item));
		execute_next ();
	}

	bool reschedule (const DeadlineTime& deadline, Item item) NIRVANA_NOEXCEPT
	{
		Item removed = queue_.find_and_delete (item);
		if (removed) {
			queue_.insert (deadline, removed);
			return true
		} else
			return false;
	}

	bool reschedule (const DeadlineTime& deadline, const ExecutorRef& executor, const DeadlineTime& deadline_prev) NIRVANA_NOEXCEPT
	{
		Queue::NodeVal keynode (1, deadline_prev, std::ref (executor));
		return reschedule (deadline, &keynode);
	}

	void core_free () NIRVANA_NOEXCEPT
	{
		free_cores_.increment ();
		execute_next ();
	}

private:
	void execute_next (); NIRVANA_NOEXCEPT

private:
	Queue queue_;
	AtomicCounter free_cores_;
};

template <class T, class ExecutorRef>
void SchedulerImpl <T, ExecutorRef>::execute_next () NIRVANA_NOEXCEPT
{
	do {

		// Acquire processor core
		for (;;) {
			if ((AtomicCounter::IntType)free_cores_.decrement () >= 0)
				break;
			if ((AtomicCounter::IntType)free_cores_.increment () <= 0)
				return;
		}

		// Get first item
		Item item = queue_.delete_min ();
		if (item) {
			ExecutorRef val = item->value ().val;
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
