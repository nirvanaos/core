// Nirvana project.
// Synchronization domain.
#ifndef NIRVANA_CORE_SYNCDOMAIN_H_
#define NIRVANA_CORE_SYNCDOMAIN_H_

#include "ExecDomain.h"
#include "PriorityQueue.h"

namespace Nirvana {
namespace Core {

class SyncDomain
{
public:
	SyncDomain () :
		schedule_cnt_ (0),
		current_executor_ (nullptr),
		min_deadline_ (0)
	{}

	void schedule (ExecDomain& ed)
	{
		queue_.insert (ed.deadline (), &ed, ed.rndgen ());
		schedule ();
	}

	void schedule ();

	DeadlineTime min_deadline () const
	{
		return min_deadline_;
	}

	void run ();

	void leave ();

private:
	PriorityQueueT <ExecDomain> queue_;
	DeadlineTime min_deadline_;
	AtomicCounter schedule_cnt_;
	volatile ExecDomain* current_executor_;
};

}
}

#endif
