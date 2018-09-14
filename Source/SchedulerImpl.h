#ifndef NIRVANA_CORE_SCHEDULER_H_
#define NIRVANA_CORE_SCHEDULER_H_

#include "PriorityQueue.h"
#include "config.h"

#ifdef _WIN32
#include "Windows/SchedulerWindows.h"
namespace Nirvana {
namespace Core {
typedef Windows::SchedulerWindows SchedulerBase;
}
}
#else
#error Unknown platform.
#endif

namespace Nirvana {
namespace Core {

class SchedulerImpl :
	private SchedulerBase
{
	friend class SchedulerBase;
public:

	void schedule (DeadlineTime deadline, const QueueItem& item, bool update);
	void core_free ();

private:
	typedef SchedulerBase::QueueItem QueueItem;

	PriorityQueue <QueueItem, PRIORITY_QUEUE_LEVELS> queue_;
};

}
}

#endif
