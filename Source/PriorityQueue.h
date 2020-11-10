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
public:
	typedef typename Base::NodeVal NodeVal;

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

	bool insert (const DeadlineTime& deadline, NodeVal* node) NIRVANA_NOEXCEPT
	{
		node->value ().deadline = deadline;
		std::pair <NodeVal*, bool> ins = Base::insert (node);
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
