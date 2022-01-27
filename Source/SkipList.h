/// \file
/// \brief Lock-free skip list.
/// 
/// Lock-free skip list based on the algorithm of Hakan Sundell and Philippas Tsigas.
/// http://www.non-blocking.com/download/SunT03_PQueue_TR.pdf
/// https://pdfs.semanticscholar.org/c9d0/c04f24e8c47324cb18ff12a39ff89999afc8.pdf

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
#ifndef NIRVANA_CORE_SKIPLIST_H_
#define NIRVANA_CORE_SKIPLIST_H_
#pragma once

#include "AtomicCounter.h"
#include "TaggedPtr.h"
#include "BackOff.h"
#include "RandomGen.h"
#include "StaticallyAllocated.h"
#include <algorithm>
#include <utility>

namespace Nirvana {
namespace Core {

/// Base class implementing lock-free skip list algorithm.
class SkipListBase
{
public:
	typedef uint_fast8_t Level;

	bool empty () NIRVANA_NOEXCEPT
	{
		Node* prev = copy_node (head ());
		Node* node = read_next (prev, 0);
		bool ret = node == tail ();
		release_node (prev);
		release_node (node);
		return ret;
	}

	struct Node;

private:
	/// NodeBase is used to determine the minimal size of node.
	struct NIRVANA_NOVTABLE NodeBase
	{
		Node* volatile prev;
		RefCounter ref_cnt;
		Level level;
		Level volatile valid_level;
		std::atomic <bool> deleted;

		NodeBase (unsigned l) NIRVANA_NOEXCEPT :
		prev (nullptr),
		level ((Level)l),
		valid_level ((Level)1),
		deleted (false)
		{}

		virtual ~NodeBase () NIRVANA_NOEXCEPT
		{}
	};

public:
	// Assume that key and value at least sizeof(void*) each.
	// So we add 3 * sizeof (void*) to the node size: next, key, and value.
	static const unsigned NODE_ALIGN = 1 << log2_ceil (sizeof (NodeBase) + 3 * sizeof (void*));
	typedef TaggedPtrT <Node, 1, NODE_ALIGN> Link;

	struct NIRVANA_NOVTABLE Node : NodeBase
	{
		/// Virtual operator < must be overridden.
		virtual bool operator < (const Node&) const NIRVANA_NOEXCEPT;

		Link::Lockable next [1];	// Variable length array.

		Node (unsigned level) NIRVANA_NOEXCEPT :
		NodeBase (level)
		{
			std::fill_n ((uintptr_t*)next, level, 0);
		}

		static constexpr size_t size (size_t node_size, unsigned level) NIRVANA_NOEXCEPT
		{
			return node_size + (level - 1) * sizeof (next [0]);
		}

		void* value () NIRVANA_NOEXCEPT
		{
			return next + level;
		}
	};

	/// Only `Node:level` member is used.
	virtual void deallocate_node (Node* node);

protected:

	SkipListBase (unsigned node_size, unsigned max_level, void* head_tail) NIRVANA_NOEXCEPT;

	/// Gets node with minimal key.
	/// \returns `Node*` if list not empty or `nullptr` otherwise.
	///          Returned Node* must be released by release_node().
	Node* get_min_node () NIRVANA_NOEXCEPT;

	/// Deletes node with minimal key.
	/// \returns `Node*` if list not empty or `nullptr` otherwise.
	///          Returned Node* must be released by release_node().
	Node* delete_min () NIRVANA_NOEXCEPT;

	// Node comparator.
	bool less (const Node& n1, const Node& n2) const NIRVANA_NOEXCEPT;

	Node* head () const NIRVANA_NOEXCEPT
	{
		return head_;
	}

	Node* tail () const NIRVANA_NOEXCEPT
	{
		return tail_;
	}

	unsigned max_level () const NIRVANA_NOEXCEPT
	{
		return head_->level;
	}

	unsigned random_level () NIRVANA_NOEXCEPT
	{
		// Geometric distribution
		return 1 + nlz (rndgen_ ());
	}

	size_t node_size (unsigned level) const NIRVANA_NOEXCEPT
	{
		return Node::size (node_size_, level);
	}

	Node* insert (Node* new_node, Node** saved_nodes) NIRVANA_NOEXCEPT;

	void release_node (Node* node) NIRVANA_NOEXCEPT;

	Node* find (const Node* keynode) NIRVANA_NOEXCEPT;
	Node* lower_bound (const Node* keynode) NIRVANA_NOEXCEPT;
	Node* upper_bound (const Node* keynode) NIRVANA_NOEXCEPT;

	Node* find_and_delete (const Node* keynode) NIRVANA_NOEXCEPT;

	/// Erase by key.
	bool erase (const Node* keynode) NIRVANA_NOEXCEPT
	{
		Node* node = find_and_delete (keynode);
		if (node)
			release_node (node);
		return node;
	}

	/// Directly removes node from list.
	/// `remove (find (keynode));` works slower then `erase (keynode);`.
	/// But it does not use the comparison and exactly removes the specified node.
	bool remove (Node* node) NIRVANA_NOEXCEPT
	{
		bool f = false;
		if (node->deleted.compare_exchange_strong (f, true)) {
			final_delete (node);
			return true;
		}
		return false;
	}

	/// Only `Node::level` member is valid on return.
	Node* allocate_node (unsigned level);

private:
	static Node* read_node (Link::Lockable& node) NIRVANA_NOEXCEPT;

	static Node* copy_node (Node* node) NIRVANA_NOEXCEPT
	{
		assert (node);
		node->ref_cnt.increment ();
		return node;
	}

	Node* read_next (Node*& node1, int level) NIRVANA_NOEXCEPT;

	Node* scan_key (Node*& node1, int level, const Node* keynode) NIRVANA_NOEXCEPT;

	Node* help_delete (Node*, int level) NIRVANA_NOEXCEPT;

	void remove_node (Node* node, Node*& prev, int level) NIRVANA_NOEXCEPT;
	void final_delete (Node* node) NIRVANA_NOEXCEPT;
	void delete_node (Node* node) NIRVANA_NOEXCEPT;

protected:
#ifdef _DEBUG
	AtomicCounter <false> node_cnt_;
#endif

private:
	Node* head_;
	Node* tail_;
	unsigned node_size_;
	RandomGenAtomic rndgen_;
};

/// Skip list implementation for the given maximal level count.
/// \tparam MAX_LEVEL Maximal level count.
template <unsigned MAX_LEVEL>
class SkipListL :
	public SkipListBase
{
	typedef SkipListBase Base;
public:
	/// Only `Node::level` member is valid on return.
	virtual Node* allocate_node ()
	{
		// Choose level randomly.
		return Base::allocate_node (random_level ());
	}

protected:
	SkipListL (unsigned node_size) NIRVANA_NOEXCEPT :
		Base (node_size, MAX_LEVEL, head_tail_)
	{}

	Node* insert (Node* new_node) NIRVANA_NOEXCEPT
	{
		Node* save_nodes [MAX_LEVEL];
		return Base::insert (new_node, save_nodes);
	}

	unsigned random_level () NIRVANA_NOEXCEPT
	{
		return MAX_LEVEL > 1 ? std::min (Base::random_level (), MAX_LEVEL) : 1;
	}

private:
	// Memory space for head and tail nodes.
	// Add extra space for tail node alignment.
	uint8_t head_tail_ [Node::size (sizeof (Node), MAX_LEVEL) + Node::size (sizeof (Node), 1) + NODE_ALIGN - 1];
};

/// Skip list implementation for the given node value type and maximal level count.
/// \tparam Val Node value type. Must have operator <.
/// \tparam MAX_LEVEL Maximal level count.
template <typename Val, unsigned MAX_LEVEL>
class SkipList :
	public SkipListL <MAX_LEVEL>
{
	typedef SkipListL <MAX_LEVEL> Base;
public:
	typedef Val Value;

	struct NodeVal : Base::Node
	{
		// Reserve space for creation of auto variables with level = 1.
		uint8_t val_space [sizeof (Val)];

		Value& value () NIRVANA_NOEXCEPT
		{
			return *(Value*)Base::Node::value ();
		}

		const Value& value () const NIRVANA_NOEXCEPT
		{
			return const_cast <NodeVal*> (this)->value ();
		}

		virtual bool operator < (const typename Base::Node& rhs) const NIRVANA_NOEXCEPT
		{
			return value () < static_cast <const NodeVal&> (rhs).value ();
		}

		template <class ... Args>
		NodeVal (int level, Args ... args) NIRVANA_NOEXCEPT :
		Base::Node (level)
		{
			new (&value ()) Value (std::forward <Args> (args)...);
		}

		~NodeVal () NIRVANA_NOEXCEPT
		{
			value ().~Val ();
		}
	};

	SkipList () NIRVANA_NOEXCEPT :
		Base (sizeof (NodeVal))
	{}

	/// Inserts new node.
	/// \param val Node value.
	/// \returns std::pair <NodeVal*, bool>.
	///          first must be released by `release_node`.
	///          second is `true` if new node was inserted, false if node already exists.
	template <class ... Args>
	std::pair <NodeVal*, bool> insert (Args ... args)
	{
		return insert (create_node (std::forward <Args> (args)...));
	}

	std::pair <NodeVal*, bool> insert (NodeVal* new_node) NIRVANA_NOEXCEPT
	{
		NodeVal* p = static_cast <NodeVal*> (Base::insert (new_node));
		return std::pair <NodeVal*, bool> (p, p == new_node);
	}

	template <class ... Args>
	NodeVal* create_node (Args ... args)
	{
		SkipListBase::Node* node = this->allocate_node ();
		// Initialize node
		return new (node) NodeVal (node->level, std::forward <Args> (args)...);
	}

	/// Gets minimal value.
	/// \param [out] val Node value.
	/// \returns `true` if node found, `false` if queue is empty.
	bool get_min_value (Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal* node = get_min_node ();
		if (node) {
			val = node->value ();
			release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with minimal value.
	/// \param [out] val Node value.
	/// \returns `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal* node = delete_min ();
		if (node) {
			val = std::move (node->value ());
			release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with specified value.
	/// \param val Node value to delete.
	/// \returns `true` if node deleted, `false` if value not found.
	bool erase (const Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal keynode (1, val);
		return Base::erase (&keynode);
	}

	template <class ... Args>
	bool erase (Args ... args)
	{
		NodeVal keynode (1, std::forward <Args> (args)...);
		return Base::erase (&keynode);
	}

	bool erase (const NodeVal* keynode) NIRVANA_NOEXCEPT
	{
		return Base::erase (keynode);
	}

	/// Gets node with minimal value.
	/// \returns Minimal node pointer or `nullptr` if list is empty.
	///          Returned pointer must be released by `release_node`.
	NodeVal* get_min_node () NIRVANA_NOEXCEPT
	{
		return static_cast <NodeVal*> (Base::get_min_node ());
	}

	/// Deletes node with minimal value.
	/// \returns Removed node pointer or `nullptr` if list is empty.
	///          Returned pointer must be released by `release_node`.
	NodeVal* delete_min () NIRVANA_NOEXCEPT
	{
		return static_cast <NodeVal*> (Base::delete_min ());
	}

	/// Releases the node pointer.
	void release_node (NodeVal* node) NIRVANA_NOEXCEPT
	{
		Base::release_node (node);
	}

	/// Finds node by value.
	/// \returns The Node pointer or `nullptr` if value not found.
	///          Returned pointer must be released by `release_node`.
	NodeVal* find (const NodeVal* keynode) NIRVANA_NOEXCEPT
	{
		return static_cast <NodeVal*> (Base::find (keynode));
	}

	/// Finds node by value.
	/// \returns The Node pointer or `nullptr` if value not found.
	///          Returned pointer must be released by `release_node`.
	NodeVal* find (const Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal keynode (1, val);
		return find (&keynode);
	}

	NodeVal* find_and_delete (const NodeVal* keynode) NIRVANA_NOEXCEPT
	{
		return static_cast <NodeVal*> (Base::find_and_delete (keynode));
	}

	NodeVal* find_and_delete (const Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal keynode (1, val);
		return find_and_delete (&keynode);
	}

	/// Finds first node equal or greater than value.
	/// \returns Node pointer or `nullptr` if node not found.
	///          Returned pointer must be released by `release_node`.
	NodeVal* lower_bound (const Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal keynode (1, val);
		return static_cast <NodeVal*> (Base::lower_bound (&keynode));
	}

	/// Finds first node greater than value.
	/// \returns Node pointer or `nullptr` if node not found.
	///          Returned pointer must be released by `release_node`.
	NodeVal* upper_bound (const Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal keynode (1, val);
		return static_cast <NodeVal*> (Base::upper_bound (&keynode));
	}

	/// Removes node from list.
	/// `remove (find (key));` works slower then `erase (key);`.
	bool remove (NodeVal* node) NIRVANA_NOEXCEPT
	{
		return Base::remove (node);
	}
};

}
}

#endif
