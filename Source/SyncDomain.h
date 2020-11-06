// Nirvana project.
// Synchronization domain.
#ifndef NIRVANA_CORE_SYNCDOMAIN_H_
#define NIRVANA_CORE_SYNCDOMAIN_H_

#include "PriorityQueue.h"
#include "Scheduler.h"
#include "SyncContext.h"
#include "CoreObject.h"
#include "RuntimeSupportImpl.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE SyncDomain :
	public CoreObject,
	public Executor,
	public SyncContext
{
	typedef PriorityQueue <ExecDomain*, SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS> Queue;
public:

	SyncDomain () :
		min_deadline_ (0),
		running_ (false)
	{}

	typedef Queue::NodeVal QueueNode;

	QueueNode* queue_node_create (DeadlineTime deadline, ExecDomain* ed)
	{
		return queue_.create_node (deadline, ed);
	}

	void queue_node_release (QueueNode* node) NIRVANA_NOEXCEPT
	{
		queue_.release_node (node);
	}

	void schedule (QueueNode* node); //  NIRVANA_NOEXCEPT

	void schedule (ExecDomain& ed);

	DeadlineTime min_deadline () const
	{
		return min_deadline_;
	}

	virtual void execute (DeadlineTime deadline, Word scheduler_error);

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
	Queue queue_;
	std::atomic <DeadlineTime> min_deadline_; // TODO: Lock-free atomic 64-bit may be unavailable!
	std::atomic <bool> running_;
	Heap heap_;
	RuntimeSupportImpl runtime_support_; // Must be destructed before the heap_ destruction.
};

}
}

#endif
