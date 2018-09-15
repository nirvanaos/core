// Nirvana project.
// Synchronization domain.
#ifndef NIRVANA_CORE_SYNCDOMAIN_H_
#define NIRVANA_CORE_SYNCDOMAIN_H_

#include "PriorityQueue.h"
#include "config.h"
#include "../Interface/Runnable.h"
#include "AtomicCounter.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class SyncDomain :
	public CoreObject,
	public ::CORBA::Nirvana::Servant <ExecContext, Runnable>
{
public:
	SyncDomain () :
		schedule_changed_ (0),
		running_ (0),
		min_deadline_ (0)
	{}

	void schedule (ExecDomain& ed)
	{
		queue_.insert (ed.deadline (), &ed);
		if ((1 == schedule_changed_.increment ()) && !schedule_.test_and_set ())
			schedule ();
	}

	DeadlineTime min_deadline () const
	{
		return min_deadline_;
	}

	void run ()
	{
		bool f = false;
		if (running_.compare_exchange_strong (f, true)) {
			// This thread becomes a worker.

			// Become a scheduler also.
			schedule_changed_ = 1;
			for (BackOff bo; schedule_.test_and_set (); bo.sleep ());
			
			min_deadline_ = 0;
			ExecDomain* executor;
			if (queue_.delete_min (executor)) {
				current_executor_ = executor;
				executor->_this ()->run ();
				current_executor_ = nullptr;
			}
			running_ = false;
			schedule ();
		}
	}

private:
	void schedule ();

private:
	PriorityQueue <ExecDomain*, SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS> queue_;
	DeadlineTime min_deadline_;
	std::atomic <bool> running_;
	AtomicCounter schedule_changed_;
	std::atomic_flag schedule_;
	volatile ExecDomain* current_executor_;
};

}
}

#endif
