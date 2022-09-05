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
#pragma once

#include "PriorityQueue.h"
#include "Scheduler.h"
#include "Executor.h"
#include "SyncContext.h"
#include "SharedObject.h"
#include "MemContext.h"
#include <atomic>

namespace Nirvana {
namespace Core {

/// Synchronization domain.
class NIRVANA_NOVTABLE SyncDomain :
	public SharedObject,
	public Executor,
	public SyncContext
{
	DECLARE_CORE_INTERFACE
	
private:
	typedef PriorityQueue <Executor*, SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS> Queue;

public:
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

	virtual SyncDomain* sync_domain () NIRVANA_NOEXCEPT;

	/// \returns Domain memory context.
	MemContext& mem_context () NIRVANA_NOEXCEPT
	{
		assert (mem_context_);
		return *mem_context_;
	}

	/// If we currently run out of SD, create new SD and enter into it.
	static SyncDomain& enter ();

	void leave () NIRVANA_NOEXCEPT;

protected:
	SyncDomain (MemContext& memory) NIRVANA_NOEXCEPT;
	~SyncDomain ();

private:
	void schedule () NIRVANA_NOEXCEPT;

	void activity_begin ();
	void activity_end () NIRVANA_NOEXCEPT;

private:
	Queue queue_;
	CoreRef <MemContext> mem_context_;

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

class SyncDomainImpl : public SyncDomain
{
protected:
	SyncDomainImpl (SyncContext& parent, MemContext& memory) NIRVANA_NOEXCEPT :
		SyncDomain (memory),
		parent_ (&parent)
	{
		assert (!parent.sync_domain ());
	}

	~SyncDomainImpl ()
	{}

	// SyncContext::
	virtual Module* module () NIRVANA_NOEXCEPT;
	virtual void raise_exception (CORBA::SystemException::Code code, unsigned minor);

private:
	// Parent free sync context.
	CoreRef <SyncContext> parent_;
};

}
}

#endif
