/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
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

/// Synchronization domain.
class NIRVANA_NOVTABLE SyncDomain :
	public CoreObject,
	public Executor,
	public SyncContext
{
protected:
	friend class CoreRef <SyncDomain>;
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

	virtual void execute (int scheduler_error);

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
	HeapUser heap_;
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

template <class T, SyncDomain* sync_domain>
class SyncAllocator :
	public std::allocator <T>
{
public:
	static void deallocate (T* p, size_t cnt)
	{
		sync_domain->heap ().release (p, cnt * sizeof (T));
	}

	static T* allocate (size_t cnt, void* hint = nullptr, unsigned flags = 0)
	{
		return (T*)sync_domain->heap ().allocate (hint, cnt * sizeof (T), flags);
	}
};

template <SyncDomain* sync_domain>
using SyncString = std::basic_string <char, std::char_traits <char>, SyncAllocator <char, sync_domain> >;

}
}

#endif
