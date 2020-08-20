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

	PriorityQueueKeyVal (DeadlineTime dt, Val v) :
		deadline (dt),
		val (v)
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
	typedef Base::NodeVal NodeVal;
public:
	bool get_min_deadline (DeadlineTime& dt)
	{
		NodeVal* node = Base::get_min_node ();
		if (node) {
			dt = node->value ().deadline;
			Base::release_node (node);
			return true;
		}
		return false;
	}

	/// Inserts new node.
	bool insert (DeadlineTime dt, const Val& value)
	{
		std::pair <NodeVal*, bool> ins = Base::insert (dt, value);
		Base::release_node (ins.first);
		return ins.second;
	}

	/// Deletes node with minimal deadline.
	/// \param [out] val Value.
	/// \return `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val)
	{
		NodeVal* node = Base::delete_min ();
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
	bool delete_min (Val& val, DeadlineTime& deadline)
	{
		NodeVal* node = Base::delete_min ();
		if (node) {
			deadline = node->value ().deadline;
			val = node->value ().val;
			Base::release_node (node);
			return true;
		}
		return false;
	}

	bool erase (DeadlineTime dt, const Val& value)
	{
		return Base::erase ({ dt, value });
	}
};

}
}

#endif
