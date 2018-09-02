// Nirvana project.
// Synchronization domain.

#include "ProtDomain.h"

namespace Nirvana {
namespace Core {

void SyncDomain::schedule ()
{
	if (1 == schedule_cnt_.increment ()) {
		// This thread became the scheduler.
		do {
			if (current_executor_) {
				if (0 == schedule_cnt_.decrement ()) {
					if (current_executor_)
						break;
					else if (1 != schedule_cnt_.increment ())
						break;
				}
			}

			DeadlineTime min_deadline;
			if (queue_.get_min_deadline (min_deadline)) {
				bool first_time = !min_deadline;
				if (first_time || (min_deadline_ > min_deadline)) {
					min_deadline_ = min_deadline;
					ProtDomain::singleton ().schedule (*this, !first_time);
				}
			}
		} while (schedule_cnt_.decrement ());
	}
}

void SyncDomain::leave ()
{
	current_executor_ = nullptr;
	schedule ();
}

}
}
