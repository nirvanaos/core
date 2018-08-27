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
		schedule_cnt_ (0)
	{}

	void schedule (ExecDomain& ed)
	{
		queue_.insert (ed.deadline (), &ed, ed.rndgen ());
	}

private:
	PriorityQueueT <ExecDomain> queue_;
	AtomicCounter schedule_cnt_;
};

}
}

#endif
