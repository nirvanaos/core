// Nirvana project.
// Synchronization domain.
#ifndef NIRVANA_CORE_SYNCDOMAIN_H_
#define NIRVANA_CORE_SYNCDOMAIN_H_

#include "PriorityQueue.h"
#include <Port/config.h>
#include "Scheduler.h"
#include "ExecDomain.h"
#include "Scheduler.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class SyncDomain :
	public CoreObject,
	public Executor,
	public ::CORBA::Nirvana::ServantTraits <SyncDomain>,
	public ::CORBA::Nirvana::LifeCycleRefCntPseudo <SyncDomain>
{
public:
	SyncDomain () :
		running_ (0),
		min_deadline_ (0)
	{}

	void schedule (ExecDomain& ed)
	{
		queue_.insert (ed.deadline (), &ed);
		schedule ();
	}

	DeadlineTime min_deadline () const
	{
		return min_deadline_;
	}

	void execute (DeadlineTime deadline)
	{
		assert (deadline);
		DeadlineTime min_deadline = min_deadline_;
		if (min_deadline && deadline >= min_deadline && min_deadline_.compare_exchange_strong (min_deadline, 0)) {
			bool f = false;
			if (running_.compare_exchange_strong (f, true)) {
				ExecDomain* executor;
				DeadlineTime dt;
				if (queue_.delete_min (executor, dt)) {
					current_executor_ = executor;
					executor->execute (dt);
					current_executor_ = nullptr;
				}
				running_ = false;
				schedule ();
			}
		}
	}

	void leave ();

private:
	void schedule ();

private:
	PriorityQueue <ExecDomain*, SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS> queue_;
	std::atomic <DeadlineTime> min_deadline_;
	std::atomic <bool> running_;
	volatile ExecDomain* current_executor_;
};

}
}

#endif
