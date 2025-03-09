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
	public Executor,
	public SyncContext
{
	DECLARE_CORE_INTERFACE
	
private:
	typedef PriorityQueue <Ref <Executor>, SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS> Queue;

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
	/// \param executor An ExecDomain reference.
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
	/// \param executor An ExecDomain reference.
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

	/// Leave this SD
	void leave () noexcept;

	virtual SyncContext::Type sync_context_type () const noexcept override;

protected:
	SyncDomain (Ref <MemContext>&& mem_context) noexcept;
	~SyncDomain ();

	SyncDomain (const SyncDomain&) = delete;
	SyncDomain (SyncDomain&&) = delete;
	SyncDomain& operator = (const SyncDomain&) = delete;
	SyncDomain& operator = (SyncDomain&&) = delete;

private:
	void schedule () noexcept
	{
		need_schedule_.store (true, std::memory_order_relaxed);
		do_schedule ();
	}

	void do_schedule () noexcept;
	void end_execute () noexcept;

	void activity_begin ();
	void activity_end () noexcept;

private:
	enum class State
	{
		IDLE,
		SCHEDULING,
		STOP_SCHEDULING,
		SCHEDULED
	};

	Queue queue_;
	Ref <MemContext> mem_context_;
	ExecDomain* executing_domain_;
	std::atomic <State> state_;
	AtomicCounter <false> activity_cnt_;
	std::atomic <bool> need_schedule_;
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

	~SyncDomainCore ()
	{}

	// SyncContext::
	virtual Module* module () noexcept;
};

template <class T>
class SyncDomainDyn final :
	public T
{
public:
	template <class ... Args>
	static Ref <SyncDomain> create (Heap& heap, Args&& ... args)
	{
		size_t cb = sizeof (SyncDomainDyn);
		SyncDomain* sd = new (heap.allocate (nullptr, cb, 0))
			SyncDomainDyn (std::forward <Args> (args)...);
		assert (&sd->mem_context ().heap () == &heap);
		return CreateRef (sd);
	}

private:
	template <class> friend class CORBA::servant_reference;

	template <class ... Args>
	SyncDomainDyn (Args&& ... args) :
		T (std::forward <Args> (args)...)
	{}

	~SyncDomainDyn ()
	{}

	void _add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () noexcept
	{
		if (!ref_cnt_.decrement ()) {
			SyncDomain& sd = static_cast <SyncDomain&> (*this);
			Ref <Heap> heap = &sd.mem_context ().heap ();
			this->~SyncDomainDyn ();
			heap->release (this, sizeof (SyncDomainDyn));
		}
	}

private:
	class CreateRef : public Ref <SyncDomain>
	{
	public:
		CreateRef (SyncDomain* p) :
			Ref <SyncDomain> (p, false)
		{}
	};

private:
	RefCounter ref_cnt_;
};

}
}

#endif
