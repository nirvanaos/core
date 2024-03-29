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
	/// \brief Pre-allocated queue node
	class QueueNode : private SkipListBase::Node
	{
	public:
		QueueNode* next () const noexcept
		{
			return next_;
		}

		void release () noexcept
		{
			domain_->release_queue_node (this);
		}

	private:
		friend class SyncDomain;

		SyncDomain* domain_;
		QueueNode* next_;
	};

	/// \brief Allocate QueueNode
	/// 
	/// \param next Next node in list.
	/// \returns QueueNode pointer.
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

	/// \brief Release QueueNode
	/// 
	/// \param node The QueueNode pointer.
	void release_queue_node (QueueNode* node) noexcept;

	/// \brief Schedule executor.
	/// May throw memory allocation exception.
	/// 
	/// \param deadline Deadline time.
	/// \param executor An ExecContext reference.
	void schedule (const DeadlineTime& deadline, Executor& executor)
	{
		activity_begin ();
		try {
			NIRVANA_VERIFY (queue_.insert (deadline, &executor));
		} catch (...) {
			activity_end ();
			throw;
		}
		schedule ();
	}

	/// Schedule executor with preallocated node.
	/// Does not throw exceptions.
	/// 
	/// \param node Preallocated node.
	/// \param deadline Deadline time.
	/// \param executor An ExecContext reference.
	void schedule (QueueNode* node, const DeadlineTime& deadline, Executor& executor) noexcept
	{
		assert (node);
		assert (node->domain_ == this);
		unsigned level = node->level;
		NIRVANA_VERIFY (queue_.insert (new (node) Queue::NodeVal (level, deadline, &executor)));
		schedule ();
	}

	/// Executor::execute ()
	/// Called from worker thread.
	virtual void execute () noexcept override;

	/// \returns Domain memory context.
	MemContext& mem_context () const noexcept
	{
		assert (mem_context_);
		return *mem_context_;
	}

	/// If we currently run out of SD, create new SD and enter into it.
	static SyncDomain& enter ();

	void leave () noexcept;

	virtual SyncContext::Type sync_context_type () const noexcept override;

protected:
	SyncDomain (Ref <MemContext>&& mem_context) noexcept;
	~SyncDomain ();

private:
	void schedule () noexcept;

	void activity_begin ();
	void activity_end () noexcept;

private:
	Queue queue_;
	Ref <MemContext> mem_context_;

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

inline SyncDomain* SyncContext::sync_domain () noexcept
{
	if (sync_domain_)
		return static_cast <SyncDomain*> (this);
	else
		return nullptr;
}

/// User sync domain implementation.
class SyncDomainUser : public SyncDomain
{
protected:
	/// Constructor.
	/// 
	/// param parent Parent free sync context.
	/// param memory Memory context.
	SyncDomainUser (SyncContext& parent, MemContext& mem_context) noexcept :
		SyncDomain (Ref <MemContext> (&mem_context)),
		parent_ (&parent)
	{
		assert (!parent.sync_domain ());
	}

	~SyncDomainUser ()
	{}

	// SyncContext::
	virtual Module* module () noexcept;
	virtual void raise_exception (CORBA::SystemException::Code code, unsigned minor);

private:
	// Parent free sync context.
	Ref <SyncContext> parent_;
};

/// Core sync domain implementation.
class SyncDomainCore : public SyncDomain
{
protected:
	/// Constructor.
	/// 
	/// \param heap The heap.
	SyncDomainCore (Heap& heap);

	// SyncContext::
	virtual Module* module () noexcept;
	virtual void raise_exception (CORBA::SystemException::Code code, unsigned minor);
};

}
}

#endif
