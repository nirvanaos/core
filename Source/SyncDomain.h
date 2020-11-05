// Nirvana project.
// Synchronization domain.
#ifndef NIRVANA_CORE_SYNCDOMAIN_H_
#define NIRVANA_CORE_SYNCDOMAIN_H_

#include "PriorityQueue.h"
#include "Scheduler.h"
#include "ExecDomain.h"
#include "Scheduler.h"
#include "SyncContext.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE SyncDomain :
	public CoreObject,
	public Executor,
	public SyncContext
{
public:
	SyncDomain () :
		min_deadline_ (0),
		running_ (false)
	{}

	void schedule (ExecDomain& ed, bool ret);

	DeadlineTime min_deadline () const
	{
		return min_deadline_;
	}

	virtual void execute (DeadlineTime deadline, Word scheduler_error);

	void call_begin ();

	virtual void enter (bool ret);
	virtual void async_call (Runnable& runnable, DeadlineTime deadline, CORBA::Nirvana::Interface_ptr environment);
	virtual bool is_free_sync_context ();
	virtual Heap& memory ();

	Heap& heap ()
	{
		return heap_;
	}

	RuntimeSupportImpl& runtime_support ()
	{
		return runtime_support_;
	}

private:
	void schedule (bool ret);

private:
	PriorityQueue <ExecDomain*, SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS> queue_;
	std::atomic <DeadlineTime> min_deadline_; // TODO: Lock-free atomic 64-bit may be unavailable!
	std::atomic <bool> running_;
	Heap heap_;
	RuntimeSupportImpl runtime_support_; // Must be destructed before the heap_ destruction.
};

}
}

#endif
