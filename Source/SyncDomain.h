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
	typedef PriorityQueue <Executor*, SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS> Queue;
public:
	SyncDomain ();
	~SyncDomain ();

	typedef Queue::QueueNode QueueNode;

	QueueNode* create_queue_node (QueueNode* next)
	{
		return queue_.create_queue_node (next);
	}

	void schedule (DeadlineTime deadline, Executor& executor)
	{
		verify (queue_.insert (deadline, &executor));
		schedule ();
	}

	void schedule (QueueNode* node, DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT
	{
		verify (queue_.insert (node, deadline, &executor));
		schedule ();
	}

	virtual void execute (Word scheduler_error);

	virtual void enter (bool ret);
	virtual SyncDomain* sync_domain ();
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
	void schedule () NIRVANA_NOEXCEPT;

private:
	Queue queue_;
	Heap heap_;
	RuntimeSupportImpl runtime_support_; // Must be destructed before the heap_ destruction.

	// Thread that acquires this flag become a scheduling thread.
	std::atomic_flag scheduling_;
	volatile bool need_schedule_;

	enum class State
	{
		IDLE,
		SCHEDULED,
		RUNNING
	};
	volatile State state_;
	volatile DeadlineTime scheduled_deadline_;
};

}
}

#endif
