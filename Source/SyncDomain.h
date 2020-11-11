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

	void schedule (const DeadlineTime& deadline, Executor& executor)
	{
		if (0 == activity_cnt_.increment ())
			Scheduler::create_item ();
		verify (queue_.insert (deadline, &executor));
		schedule ();
	}

	/// Pre-allocated queue node
	class QueueNode : private SkipListBase::NodeBase
	{
	public:
		QueueNode* next () const NIRVANA_NOEXCEPT
		{
			return next_;
		}

		void release () NIRVANA_NOEXCEPT
		{
			domain_->release_queue_node (this);
		}

	private:
		friend class SyncDomain;

		SyncDomain* domain_;
		QueueNode* next_;
	};

	QueueNode* create_queue_node (QueueNode* next);
	void release_queue_node (QueueNode* node) NIRVANA_NOEXCEPT;

	void schedule (QueueNode* node, const DeadlineTime& deadline, Executor& executor) NIRVANA_NOEXCEPT
	{
		assert (node);
		assert (node->domain_ == this);
		unsigned level = node->level;
		verify (queue_.insert (new (node) Queue::NodeVal (level, deadline, &executor)));
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
	void activity_end () NIRVANA_NOEXCEPT;

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

	AtomicCounter activity_cnt_;
};

}
}

#endif
