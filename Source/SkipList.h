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
#include "LockablePtr.h"
#include "BackOff.h"
#include "RandomGen.h"
#include <algorithm>
#include <utility>
#include <Port/config.h>

namespace Nirvana {
namespace Core {

class Heap;

/// Base class implementing lock-free skip list algorithm.
class SkipListBase
{
public:
	typedef uint_fast8_t Level;

	bool empty () noexcept;

	struct Node;

	Heap& heap () const noexcept
	{
		return heap_;
	}

private:
	/// NodeBase is used to determine the minimal size of node.
	struct NIRVANA_NOVTABLE NodeBase
	{
		Node* volatile prev;
		RefCounter ref_cnt;
		Level level;
		Level volatile valid_level;
		std::atomic <bool> deleted;

		NodeBase (unsigned l) noexcept :
			prev (nullptr),
			level ((Level)l),
			valid_level ((Level)0),
			deleted (false)
		{}

		virtual ~NodeBase () noexcept
		{}
	};

public:
	static const unsigned NODE_ALIGN = LockablePtrImpl::USE_SHIFT ?
		// Assume that key and value at least sizeof(void*) each.
		// So we add 3 * sizeof (void*) to the node size: next, key, and value.
		1 << log2_ceil (sizeof (NodeBase) + 3 * sizeof (void*))
		: 
		(unsigned)alignof (NodeBase); // We don't use alignment for locking, just use 1 bit for tag.

	typedef TaggedPtrT <Node, 1, NODE_ALIGN> Link;

	struct NIRVANA_NOVTABLE Node : NodeBase
	{
		/// Virtual operator < must be overridden.
		virtual bool operator < (const Node&) const noexcept;

		Link::Lockable next [1];	// Variable length array.

		void init_next (Link link) noexcept
		{
			Link::Lockable* p = next, * end = p + level;
			do {
				new (p) Link::Lockable (link);
			} while (end != ++p);
		}

		Node (unsigned level) noexcept :
			NodeBase (level)
		{
			init_next (Link (nullptr, 1));
		}

		// Head node constructor
		Node (unsigned level, Node* tail) noexcept :
			NodeBase (level)
		{
			init_next (Link (tail));
			valid_level = level;
		}

		static constexpr size_t size (size_t node_size, unsigned level) noexcept
		{
			return round_up (sizeof (NodeBase) + level * sizeof (next [0]), alignof (max_align_t))
				+ node_size - sizeof (NodeBase);
		}

		void* value () noexcept
		{
			return round_up ((uint8_t*)(next + level), alignof (max_align_t));
		}
	};

	/// Only `Node:level` member is used.
	virtual void deallocate_node (Node* node) noexcept;

	/// Increment node reference counter.
	/// 
	/// \param node The node pointer.
	/// \return The node pointer.
	static Node* copy_node (Node* node) noexcept
	{
		assert (node);
		node->ref_cnt.increment ();
		return node;
	}

	/// Release node reference counter.
	/// 
	/// \param node The node pointer.
	void release_node (Node* node) noexcept;

protected:
	SkipListBase (Heap& heap, unsigned node_size, unsigned max_level, void* head_tail) noexcept;

	static bool pre_deallocate_node (SkipListBase::Node* node) noexcept
	{
		return true;
	}

	/// Gets node with minimal key.
	/// \returns `Node*` if list not empty or `nullptr` otherwise.
	///          Returned Node* must be released by release_node().
	Node* get_min_node () noexcept;

	Node* next (Node* cur) noexcept;

	/// Deletes node with minimal key.
	/// \returns `Node*` if list not empty or `nullptr` otherwise.
	///          Returned Node* must be released by release_node().
	Node* delete_min () noexcept;

	// Node comparator.
	bool less (const Node& n1, const Node& n2) const noexcept;

	Node* head () const noexcept
	{
		return head_;
	}

	Node* tail () const noexcept
	{
		return tail_;
	}

	unsigned max_level () const noexcept
	{
		return head_->level;
	}

	unsigned random_level () noexcept
	{
		// Geometric distribution
		return 1 + nlz (rndgen_ ());
	}

	size_t node_size (unsigned level) const noexcept
	{
		return std::max (Node::size (node_size_, level), (size_t)(NODE_ALIGN / 2 + 1));
	}

	Node* insert (Node* new_node, Node** saved_nodes) noexcept;

	Node* find (const Node* keynode) noexcept;
	Node* lower_bound (const Node* keynode) noexcept;
	Node* upper_bound (const Node* keynode) noexcept;

	Node* find_and_delete (const Node* keynode) noexcept;

	/// Erase by key.
	bool erase (const Node* keynode) noexcept
	{
		Node* node = find_and_delete (keynode);
		if (node)
			release_node (node);
		return node;
	}

	/// Directly removes node from list.
	/// `remove (find (keynode));` works slower then `erase (keynode);`.
	/// But it does not use the comparison and exactly removes the specified node.
	bool remove (Node* node) noexcept;

	/// Only `Node::level` member is valid on return.
	Node* allocate_node (unsigned level);

	SkipListBase& operator = (SkipListBase&& other) noexcept
	{
		assert (node_size_ == other.node_size_);
		rndgen_ = other.rndgen_;
		return *this;
	}

private:
	static Node* read_node (Link::Lockable& node) noexcept;
	Node* read_next (Node*& node1, int level) noexcept;
	Node* scan_key (Node*& node1, int level, const Node* keynode) noexcept;
	Node* help_delete (Node*, int level) noexcept;
	void remove_node (Node* node, Node*& prev, int level) noexcept;
	void final_delete (Node* node) noexcept;
	void mark_next_links (Node* node, int from_level) noexcept;
	inline void delete_node (Node* node) noexcept;

#ifndef NDEBUG
public:
	AtomicCounter <false>::IntegralType dbg_node_cnt () const noexcept
	{
		return node_cnt_;
	}

protected:
	AtomicCounter <false> node_cnt_;
#endif

private:
	Node* const tail_;
	Node* const head_;
	Heap& heap_;
	const unsigned node_size_;
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
	virtual Node* allocate_node ();

protected:
	SkipListL (Heap& heap, unsigned node_size) noexcept;

	Node* insert (Node* new_node) noexcept;

	unsigned random_level () noexcept
	{
		return MAX_LEVEL > 1 ? std::min (Base::random_level (), MAX_LEVEL) : 1;
	}

	SkipListL& operator = (SkipListL&& other) noexcept;

private:
	// Memory space for head and tail nodes.
	// Added extra space for tail node alignment.
	struct alignas (Node) HeadTailSpace
	{
		uint8_t space [Node::size (sizeof (Node), MAX_LEVEL) + Node::size (sizeof (Node), 1) + NODE_ALIGN - 1];
	};
	HeadTailSpace head_tail_;
};

template <unsigned MAX_LEVEL>
SkipListL <MAX_LEVEL>::SkipListL (Heap& heap, unsigned node_size) noexcept :
	Base (heap, node_size, MAX_LEVEL, &head_tail_)
{
	assert ((const uint8_t*)tail () + Node::size (sizeof (Node), 1) <=
		head_tail_.space + sizeof (head_tail_.space));
}

template <unsigned MAX_LEVEL>
SkipListBase::Node* SkipListL <MAX_LEVEL>::allocate_node ()
{
	// Choose level randomly.
	return Base::allocate_node (random_level ());
}

template <unsigned MAX_LEVEL>
SkipListBase::Node* SkipListL <MAX_LEVEL>::insert (Node* new_node) noexcept
{
	Node* save_nodes [MAX_LEVEL];
	return Base::insert (new_node, save_nodes);
}

template <unsigned MAX_LEVEL>
SkipListL <MAX_LEVEL>& SkipListL <MAX_LEVEL>::operator = (SkipListL&& other) noexcept
{
	Base::operator = (std::move (other));

	// Clear this list
	while (Node* node = delete_min ()) {
		release_node (node);
	}

	// Move nodes
	while (Node* node = other.delete_min ()) {
		node->deleted = false;
		node = insert (node);
		release_node (node);
	}

#ifndef NDEBUG
	new (&node_cnt_) AtomicCounter <false> ((AtomicCounter <false>::IntegralType)other.node_cnt_);
	new (&other.node_cnt_) AtomicCounter <false> (0);
#endif

	return *this;
}

/// Skip list implementation for the given node value type and maximal level count.
/// \tparam Val Node value type. Must have operator <.
/// \tparam MAX_LEVEL Maximal level count.
template <typename Val, unsigned MAX_LEVEL = SKIP_LIST_DEFAULT_LEVELS>
class SkipList :
	public SkipListL <MAX_LEVEL>
{
	typedef SkipListL <MAX_LEVEL> Base;

public:
	typedef Val Value;

	struct NodeVal : Base::Node
	{
		// Reserve space for creation of auto variables with level = 1.
		typename std::aligned_storage <sizeof (Val)>::type val_space;

		Value& value () noexcept
		{
			return *(Value*)Base::Node::value ();
		}

		const Value& value () const noexcept
		{
			return const_cast <NodeVal*> (this)->value ();
		}

		virtual bool operator < (const typename Base::Node& rhs) const noexcept
		{
			return value () < static_cast <const NodeVal&> (rhs).value ();
		}

		template <class ... Args>
		NodeVal (int level, Args&& ... args) noexcept :
			Base::Node (level)
		{
			new (&value ()) Value (std::forward <Args> (args)...);
		}

		~NodeVal () noexcept
		{
			value ().~Val ();
		}
	};

	SkipList (Heap& heap) noexcept :
		Base (heap, sizeof (NodeVal))
	{}

	/// Inserts new node.
	/// \param val Node value.
	/// \returns std::pair <NodeVal*, bool>.
	///          first must be released by `release_node`.
	///          second is `true` if new node was inserted, false if node already exists.
	template <class ... Args>
	std::pair <NodeVal*, bool> insert (Args&& ... args)
	{
		return insert (create_node (std::forward <Args> (args)...));
	}

	std::pair <NodeVal*, bool> insert (NodeVal* new_node) noexcept
	{
		NodeVal* p = static_cast <NodeVal*> (Base::insert (new_node));
		return std::pair <NodeVal*, bool> (p, p == new_node);
	}

	template <class ... Args>
	NodeVal* create_node (Args&& ... args)
	{
		SkipListBase::Node* node = this->allocate_node ();
		// Initialize node
		return new (node) NodeVal (node->level, std::forward <Args> (args)...);
	}

	/// Gets minimal value.
	/// \param [out] val Node value.
	/// \returns `true` if node found, `false` if queue is empty.
	bool get_min_value (Val& val) noexcept
	{
		NodeVal* node = get_min_node ();
		if (node) {
			val = node->value ();
			Base::release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with minimal value.
	/// \param [out] val Node value.
	/// \returns `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val) noexcept
	{
		NodeVal* node = delete_min ();
		if (node) {
			val = std::move (node->value ());
			Base::release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with specified value.
	/// \param val Node value to delete.
	/// \returns `true` if node deleted, `false` if value not found.
	bool erase (const Val& val) noexcept
	{
		NodeVal keynode (1, std::ref (val));
		return Base::erase (&keynode);
	}

	bool erase (const NodeVal* keynode) noexcept
	{
		return Base::erase (keynode);
	}

	/// Gets node with minimal value.
	/// \returns Minimal node pointer or `nullptr` if list is empty.
	///          Returned pointer must be released by `release_node`.
	NodeVal* get_min_node () noexcept
	{
		return static_cast <NodeVal*> (Base::get_min_node ());
	}

	NodeVal* next (NodeVal* cur) noexcept
	{
		return static_cast <NodeVal*> (Base::next (cur));
	}

	/// Deletes node with minimal value.
	/// \returns Removed node pointer or `nullptr` if list is empty.
	///          Returned pointer must be released by `release_node`.
	NodeVal* delete_min () noexcept
	{
		return static_cast <NodeVal*> (Base::delete_min ());
	}

	/// Finds node by value.
	/// \returns The Node pointer or `nullptr` if value not found.
	///          Returned pointer must be released by `release_node`.
	NodeVal* find (const NodeVal* keynode) noexcept
	{
		return static_cast <NodeVal*> (Base::find (keynode));
	}

	/// Finds node by value.
	/// \returns The Node pointer or `nullptr` if value not found.
	///          Returned pointer must be released by `release_node`.
	template <class ... Args>
	NodeVal* find (Args&& ... args)
	{
		NodeVal keynode (1, std::forward <Args> (args)...);
		return static_cast <NodeVal*> (Base::find (&keynode));
	}

	NodeVal* find_and_delete (const NodeVal* keynode) noexcept
	{
		return static_cast <NodeVal*> (Base::find_and_delete (keynode));
	}

	NodeVal* find_and_delete (const Val& val) noexcept
	{
		NodeVal keynode (1, std::ref (val));
		return find_and_delete (&keynode);
	}

	/// Finds first node equal or greater than value.
	/// \returns Node pointer or `nullptr` if node not found.
	///          Returned pointer must be released by `release_node`.
	NodeVal* lower_bound (const Val& val) noexcept
	{
		NodeVal keynode (1, std::ref (val));
		return static_cast <NodeVal*> (Base::lower_bound (&keynode));
	}

	/// Finds first node greater than value.
	/// \returns Node pointer or `nullptr` if node not found.
	///          Returned pointer must be released by `release_node`.
	NodeVal* upper_bound (const Val& val) noexcept
	{
		NodeVal keynode (1, std::ref (val));
		return static_cast <NodeVal*> (Base::upper_bound (&keynode));
	}

	/// Removes node from list.
	/// `remove (find (key));` works slower then `erase (key);`.
	/// But it does not use the comparison and exactly removes the specified node.
	bool remove (NodeVal* node) noexcept
	{
		return Base::remove (node);
	}

	SkipList& operator = (SkipList&& other) noexcept
	{
		Base::operator = (std::move (other));
		return *this;
	}
};

}
}

#endif
