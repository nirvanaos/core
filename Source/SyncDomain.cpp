// Nirvana project.
// Synchronization domain.

#include "SyncDomain.h"

namespace Nirvana {
namespace Core {

void SyncDomain::schedule ()
{
	if (!running_) {
		DeadlineTime min_deadline;
		if (queue_.get_min_deadline (min_deadline)) {
			DeadlineTime prev_deadline = min_deadline_;
			if (
				(!prev_deadline || (prev_deadline != min_deadline))
				&&
				min_deadline_.compare_exchange_strong (prev_deadline, min_deadline)
				&&
				!running_
				)
				g_scheduler->schedule (min_deadline, _this (), prev_deadline);
		}
	}
}

}
}
