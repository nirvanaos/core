// Nirvana project.
// Synchronization domain.
#ifndef NIRVANA_CORE_SYNCDOMAIN_H_
#define NIRVANA_CORE_SYNCDOMAIN_H_

#include "ExecDomain.h"
#include "PriorityQueue.h"
#include "Thread.h"
#include "config.h"

namespace Nirvana {
namespace Core {

class SyncDomain :
	public CoreObject
{
public:
	SyncDomain () :
		schedule_cnt_ (0),
		current_executor_ (nullptr),
		min_deadline_ (0)
	{}

	void schedule (ExecDomain& ed)
	{
		queue_.insert (ed.deadline (), &ed, Thread::current ()->rndgen ());
		schedule ();
	}

	void schedule ();

	DeadlineTime min_deadline () const
	{
		return min_deadline_;
	}

	void execute ()
	{
		ExecDomain* executor = queue_.delete_min ();
		if (executor) {
			current_executor_ = executor;
			executor->switch_to ();
		}
	}

	void leave ();

private:
	PriorityQueueT <ExecDomain, PRIORITY_QUEUE_LEVELS> queue_;
	DeadlineTime min_deadline_;
	AtomicCounter schedule_cnt_;
	volatile ExecDomain* current_executor_;
};

}
}

#endif
