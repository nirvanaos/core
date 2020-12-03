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
protected:
	friend class Core_var <SyncDomain>;
	virtual void _add_ref () = 0;
	virtual void _remove_ref () = 0;

private:
	typedef PriorityQueue <Executor*, SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS> Queue;

public:
	SyncDomain ();
	~SyncDomain ();

	void schedule (const DeadlineTime& deadline, Executor& executor)
	{
		activity_begin ();
		try {
			verify (queue_.insert (deadline, &executor));
		} catch (...) {
			activity_end ();
			throw;
		}
		schedule ();
	}

	/// Pre-allocated queue node
	class QueueNode : private SkipListBase::Node
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

	QueueNode* create_queue_node (QueueNode* next)
	{
		activity_begin ();
		try {
			QueueNode* node = static_cast <QueueNode*> (queue_.allocate_node ());
			node->domain_ = this;
			node->next_ = next;
			return node;
		} catch (...) {
			activity_end ();
			throw;
		}
	}

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

	virtual void schedule_call (SyncDomain* sync_domain);
	virtual void schedule_return (ExecDomain& exec_domain) NIRVANA_NOEXCEPT;

	virtual SyncDomain* sync_domain () NIRVANA_NOEXCEPT;
	virtual Heap& memory () NIRVANA_NOEXCEPT;

	Heap& heap ()
	{
		return heap_;
	}

	RuntimeSupportImpl& runtime_support ()
	{
		return runtime_support_;
	}

	/// If we currently run out of SD, create new SD and enter into it.
	static void enter ();

	void leave () NIRVANA_NOEXCEPT;

private:
	void schedule () NIRVANA_NOEXCEPT;

	void activity_begin ()
	{
		if (1 == activity_cnt_.increment ())
			Scheduler::create_item ();
	}

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

	AtomicCounter <false> activity_cnt_;
};

}
}

#endif
