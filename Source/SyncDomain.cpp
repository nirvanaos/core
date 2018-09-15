// Nirvana project.
// Synchronization domain.

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

void SyncDomain::schedule ()
{
	do {
		schedule_changed_ = 1;
		DeadlineTime min_deadline;
		if (queue_.get_min_deadline (min_deadline)) {
			if (running_)
				break;
			DeadlineTime prev_deadline = min_deadline_;
			if (!prev_deadline || (prev_deadline > min_deadline)) {
				min_deadline_ = min_deadline;
				g_scheduler->schedule (min_deadline, _this (), prev_deadline);
			}
		}
	} while (schedule_changed_.decrement ());
	schedule_.clear ();
}

}
}
