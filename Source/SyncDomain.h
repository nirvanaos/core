// Nirvana project.
// Synchronization domain.
#ifndef NIRVANA_CORE_SYNCDOMAIN_H_
#define NIRVANA_CORE_SYNCDOMAIN_H_

#include "PriorityQueue.h"
#include "Thread.h"
#include "config.h"
#include "../Interface/Runnable.h"

namespace Nirvana {
namespace Core {

class SyncDomain :
	public CoreObject,
	public ::CORBA::Nirvana::Servant <ExecContext, Runnable>
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

	void run ()
	{
		ExecDomain* executor;
		if (queue_.delete_min (executor)) {
			current_executor_ = executor;
			executor->_this()->run ();
		}
	}

	void leave ();

private:
	PriorityQueue <ExecDomain*, PRIORITY_QUEUE_LEVELS> queue_;
	DeadlineTime min_deadline_;
	AtomicCounter schedule_cnt_;
	volatile ExecDomain* current_executor_;
};

}
}

#endif
