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
#ifndef NIRVANA_CORE_PRIORITYQUEUEREORDER_H_
#define NIRVANA_CORE_PRIORITYQUEUEREORDER_H_
#pragma once

#include "SkipList.h"
#include "Heap.h"

namespace Nirvana {
namespace Core {

template <class Val>
struct PriorityQueueReorderKeyVal
{
	DeadlineTime deadline;
	Val val;
	SkipListBase::Node* first_node;
	RefCounter ref_cnt1;
	std::atomic_flag dispatched;

	PriorityQueueReorderKeyVal (const DeadlineTime& dt, const Val& v, SkipListBase::Node* first = nullptr) :
		deadline (dt),
		val (v),
		first_node (first),
		dispatched ATOMIC_FLAG_INIT
	{}

	PriorityQueueReorderKeyVal (const DeadlineTime& dt, Val&& v) :
		deadline (dt),
		val (std::move (v)),
		first_node (nullptr),
		dispatched ATOMIC_FLAG_INIT
	{}

	bool operator < (const PriorityQueueReorderKeyVal& rhs) const noexcept
	{
		if (deadline < rhs.deadline)
			return true;
		else if (deadline > rhs.deadline)
			return false;
		else
			return val < rhs.val;
	}

};

template <typename Val, unsigned MAX_LEVEL = SKIP_LIST_DEFAULT_LEVELS>
class PriorityQueueReorder :
	public SkipList <PriorityQueueReorderKeyVal <Val>, MAX_LEVEL>
{
	typedef SkipList <PriorityQueueReorderKeyVal <Val>, MAX_LEVEL> Base;
	typedef PriorityQueueReorder <Val, MAX_LEVEL> Queue;

public:
	typedef typename Base::NodeVal NodeVal;
	typedef typename Base::Value Value;

	PriorityQueueReorder () :
		Base (Heap::core_heap ())
	{}

	~PriorityQueueReorder ()
	{
#ifndef NDEBUG
		assert (!Base::node_cnt_);
#endif
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

	bool reorder (const DeadlineTime& deadline, const Val& val, const DeadlineTime& old) noexcept
	{
		assert (deadline != old);
		NodeVal* old_node = Base::find (std::ref (old), std::ref (val));
		if (old_node) {
			SkipListBase::Node* first_node = old_node->value ().first_node;
			if (!first_node)
				first_node = old_node;
			static_cast <NodeVal&> (*first_node).value ().ref_cnt1.increment ();
			try {
				std::pair <NodeVal*, bool> ins = Base::insert (std::ref (deadline), std::ref (val), first_node);
				assert (ins.second);
				// New inserted, try remove old
				bool ret = Base::remove (old_node);
				if (!ret) // Old disappeared, try remove new
					Base::remove (ins.first);
				Base::release_node (old_node);
				Base::release_node (ins.first);
				return ret;
			} catch (...) {
				deallocate_node (first_node); // first_node->value ().ref_cnt1.decrement ();
				Base::release_node (old_node);
			}
		}
		return false;
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

	virtual void deallocate_node (SkipListBase::Node* node) noexcept override
	{
		if (pre_deallocate_node (node))
			Base::deallocate_node (node);
	}

protected:
	bool pre_deallocate_node (SkipListBase::Node* node) noexcept
	{
		typename Base::Value& val = static_cast <NodeVal&> (*node).value ();
		SkipListBase::Node* first = val.first_node;
		val.first_node = nullptr;
		if (first)
			deallocate_node (first);
		return 0 == val.ref_cnt1.decrement ();
	}

private:
	/// Deletes node with minimal deadline.
	/// \return The deleted Node pointer if node deleted or `nullptr` if the queue is empty.
	NodeVal* delete_min () noexcept
	{
		NodeVal* node;
		while ((node = Base::delete_min ())) {
			SkipListBase::Node* first_node = node->value ().first_node;
			NodeVal& first = first_node ? static_cast <NodeVal&> (*first_node) : *node;
			bool dispatched = first.value ().dispatched.test_and_set ();
			if (!dispatched)
				break;
			Base::release_node (node);
		}
		return node;
	}
};

}
}

#endif
