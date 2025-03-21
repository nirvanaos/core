/// \file
/// Lock-free priority queue.
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
#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_
#pragma once

#include "SkipList.h"

namespace Nirvana {
namespace Core {

template <class Val>
struct PriorityQueueKeyVal
{
	DeadlineTime deadline;
	Val val;

	PriorityQueueKeyVal (const DeadlineTime& dt, const Val& v) :
		deadline (dt),
		val (v)
	{}

	PriorityQueueKeyVal (const DeadlineTime& dt, Val&& v) :
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

	PriorityQueue (Heap& heap) :
		Base (heap)
	{}

	~PriorityQueue ()
	{
#ifndef NDEBUG
		assert (!Base::node_cnt_);
#endif
	}

	bool empty () noexcept
	{
		NodeVal* node = Base::get_min_node ();
		if (node) {
			Base::release_node (node);
			return false;
		}
		return true;
	}

	bool get_min_deadline (DeadlineTime& dt) noexcept
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
		std::pair <NodeVal*, bool> ins = Base::insert (std::ref (deadline), std::ref (val));
		Base::release_node (ins.first);
		return ins.second;
	}

	/// Inserts preallocated node
	bool insert (NodeVal* new_node) noexcept
	{
		std::pair <NodeVal*, bool> ins = Base::insert (new_node);
		Base::release_node (ins.first);
		return ins.second;
	}

	/// Deletes node with minimal deadline.
	/// \param [out] val Value.
	/// \return `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val) noexcept
	{
		NodeVal* node = delete_min ();
		if (node) {
			val = std::move (node->value ().val);
			Base::release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with minimal deadline.
	/// \param [out] val Value.
	/// \param [out] deadline Deadline.
	/// \return `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val, DeadlineTime& deadline) noexcept
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

private:
	/// Deletes node with minimal deadline.
	/// \return The deleted Node pointer if node deleted or `nullptr` if the queue is empty.
	NodeVal* delete_min () noexcept
	{
		return Base::delete_min ();
	}
};

}
}

#endif
