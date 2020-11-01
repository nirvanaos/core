#ifndef NIRVANA_CORE_SCHEDULERIMPL_H_
#define NIRVANA_CORE_SCHEDULERIMPL_H_

#include "PriorityQueue.h"
#include "Thread.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

template <class T, class QueueItem>
class SchedulerImpl
{
public:
	SchedulerImpl (AtomicCounter::UIntType cores) :
		free_cores_ (cores)
	{}

	void schedule (DeadlineTime deadline, const QueueItem& item, DeadlineTime deadline_prev)
	{
		assert (deadline_prev != deadline);
		try {
			verify (queue_.insert (deadline, item));
		} catch (...) {
			if (deadline_prev)
				queue_.erase (deadline_prev, item);
			throw;
		}
		if (deadline_prev)
			queue_.erase (deadline_prev, item);
		execute_next ();
	}

	void core_free ()
	{
		free_cores_.increment ();
		execute_next ();
	}

	void execute_next ();

private:
	PriorityQueue <QueueItem, SYS_DOMAIN_PRIORITY_QUEUE_LEVELS> queue_;
	AtomicCounter free_cores_;
};

template <class T, class QueueItem>
void SchedulerImpl <T, QueueItem>::execute_next ()
{
	do {

		// Acquire processor core
		for (;;) {
			if ((AtomicCounter::IntType)free_cores_.decrement () >= 0)
				break;
			if ((AtomicCounter::IntType)free_cores_.increment () <= 0)
				return;
		}

		// Get next item
		QueueItem item;
		DeadlineTime deadline;
		if (queue_.delete_min (item, deadline)) {
			static_cast <T*> (this)->execute (item, deadline);
			break;
		}

		// Release processor core
		free_cores_.increment ();

	} while (!queue_.empty ());
}

}
}

#endif
