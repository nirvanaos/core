// Nirvana project
// Lock-free priority queue.

#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

#include "SkipList.h"

namespace Nirvana {
namespace Core {

template <class Val>
struct PriorityQueueKeyVal
{
	DeadlineTime deadline;
	Val val;

	PriorityQueueKeyVal (DeadlineTime dt, const Val& v) :
		deadline (dt),
		val (v)
	{}

	PriorityQueueKeyVal (DeadlineTime dt, Val&& v) :
		deadline (dt),
		val (std::move (v))
	{}

	bool operator < (const PriorityQueueKeyVal& rhs) const
	{
		if (deadline < rhs.deadline)
			return true;
		else if (deadline > rhs.deadline)
			return false;
		else
			return val < rhs.val;
	}
};

template <typename Val, unsigned MAX_LEVEL>
class PriorityQueue :
	public SkipList <PriorityQueueKeyVal <Val>, MAX_LEVEL>
{
	typedef SkipList <PriorityQueueKeyVal <Val>, MAX_LEVEL> Base;
	typedef PriorityQueue <Val, MAX_LEVEL> Queue;
public:
	typedef typename Base::NodeVal NodeVal;
	typedef typename Base::Value Value;

	~PriorityQueue ()
	{
#ifdef _DEBUG
		assert (!Base::node_cnt_);
#endif
	}

	bool get_min_deadline (DeadlineTime& dt) NIRVANA_NOEXCEPT
	{
		NodeVal* node = Base::get_min_node ();
		if (node) {
			dt = node->value ().deadline;
			Base::release_node (node);
			return true;
		}
		return false;
	}

	/// Inserts a new node.
	bool insert (const DeadlineTime& deadline, const Val& val)
	{
		std::pair <NodeVal*, bool> ins = Base::insert (deadline, std::ref (val));
		Base::release_node (ins.first);
		return ins.second;
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
			queue_->release_queue_node (this);
		}

		unsigned level () const NIRVANA_NOEXCEPT
		{
			return SkipListBase::NodeBase::level;
		}

		SkipListBase* queue () const NIRVANA_NOEXCEPT
		{
			return queue_;
		}

	private:
		friend class Queue;

		Queue* queue_;
		QueueNode* next_;
	};

	QueueNode* create_queue_node (QueueNode* next)
	{
		QueueNode* node = static_cast <QueueNode*> (Base::allocate_node ());
		node->queue_ = this;
		node->next_ = next;
		return node;
	}

	void release_queue_node (QueueNode* node) NIRVANA_NOEXCEPT
	{
		Base::deallocate_node (node);
	}

	bool insert (QueueNode* node, const DeadlineTime& deadline, const Val& val) NIRVANA_NOEXCEPT
	{
		assert (node);
		assert (node->queue () == this);
		unsigned level = node->level ();
		std::pair <NodeVal*, bool> ins = Base::insert (new (node) NodeVal (level, deadline, val));
		Base::release_node (ins.first);
		return ins.second;
	}

	/// Deletes node with minimal deadline.
	/// \return The deleted Node pointer if node deleted or `nullptr` if the queue is empty.
	NodeVal* delete_min () NIRVANA_NOEXCEPT
	{
		return Base::delete_min ();
	}

	/// Deletes node with minimal deadline.
	/// \param [out] val Value.
	/// \return `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal* node = delete_min ();
		if (node) {
			val = node->value ().val;
			Base::release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with minimal deadline.
	/// \param [out] val Value.
	/// \param [out] deadline Deadline.
	/// \return `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val, DeadlineTime& deadline) NIRVANA_NOEXCEPT
	{
		NodeVal* node = delete_min ();
		if (node) {
			deadline = node->value ().deadline;
			val = std::move (node->value ().val);
			Base::release_node (node);
			return true;
		}
		return false;
	}
};

}
}

#endif
