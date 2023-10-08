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
#include "pch.h"
#include "HeapUser.h"
#include "SkipList.h"

namespace Nirvana {
namespace Core {

#ifdef _DEBUG
#define CHECK_VALID_LEVEL(lev, nod) assert(lev < nod->level)
#else
#define CHECK_VALID_LEVEL(lev, nod)
#endif

bool SkipListBase::Node::operator < (const Node&) const noexcept
{
	// Must be overridden.
	// Base method must never be called.
	NIRVANA_UNREACHABLE_CODE ();
	return false;
}

bool SkipListBase::less (const Node& n1, const Node& n2) const noexcept
{
	if (&n1 == &n2)
		return false;

	if (head () == &n1 || tail () == &n2)
		return true;

	if (tail () == &n1 || head () == &n2)
		return false;

	return n1 < n2; // Call virtual comparator
}

SkipListBase::SkipListBase (unsigned node_size, unsigned max_level, void* head_tail) noexcept :
#ifdef _DEBUG
	node_cnt_ (0),
#endif
	head_ (new (head_tail) Node (max_level)),
	tail_ (new (round_up ((uint8_t*)head_tail + Node::size (sizeof (Node), max_level), NODE_ALIGN)) Node (1)),
	node_size_ (node_size)
{
	std::fill_n (head_->next, max_level, tail_);
	head_->valid_level = max_level;
}

SkipListBase::Node* SkipListBase::allocate_node (unsigned level)
{
	size_t cb = node_size (level);
	Node* p = (Node*)Heap::core_heap ().allocate (0, cb, 0);
	p->level = (Level)level;
#ifdef _DEBUG
	node_cnt_.increment ();
#endif
	return p;
}

void SkipListBase::deallocate_node (Node* node) noexcept
{
#ifdef _DEBUG
	node_cnt_.decrement ();
#endif
	Heap::core_heap ().release (node, node_size (node->level));
}

inline
void SkipListBase::delete_node (Node* node) noexcept
{
	node->~Node ();
	deallocate_node (node);
}

void SkipListBase::release_node (Node* node) noexcept
{
	assert (node);
	if (!node->ref_cnt.decrement ()) {
		Node* prev = node->prev;
		if (prev)
			release_node (prev);
#ifdef _DEBUG
		assert (node != head ());
		assert (node != tail ());
		for (int i = 0, end = node->level; i < end; ++i)
			assert (!(Node*)node->next [i].load ());
#endif
		delete_node (node);
	}
}

SkipListBase::Node* SkipListBase::insert (Node* const new_node, Node** saved_nodes) noexcept
{
	// In case we insert back the removed node.
	// We mustn't insert the removed node back because it may be still referenced by other nodes.
	assert (!new_node->deleted);

	int level = new_node->level;
	copy_node (new_node);

	// Search phase to find the node after which newNode should be inserted.
	// This search phase starts from the head node at the highest
	// level and traverses down to the lowest level until the correct
	// node is found (node1). When going down one level, the last
	// node traversed on that level is remembered (savedNodes)
	// for later use (this is where we should insert the new node at that level).
	Node* node1 = copy_node (head ());
	for (int i = max_level () - 1; i >= 1; --i) {
		Node* node2 = scan_key (node1, i, new_node);
		release_node (node2);
		if (i < level)
			saved_nodes [i] = copy_node (node1);
	}

	for (BackOff bo; true; bo ()) {
		Node* node2 = scan_key (node1, 0, new_node);
		if (!node2->deleted && !less (*new_node, *node2)) {
			// The same value already exists.
			release_node (node1);
			for (int i = 1; i < level; ++i)
				release_node (saved_nodes [i]);
			release_node (new_node);
			release_node (new_node);
			return node2;
		}

		// Insert node to list at level 0.
		new_node->next [0] = node2;
		release_node (node2);
		if (node1->next [0].cas (node2, new_node)) {
			release_node (node1);
			break;
		}
	}

	// After the new node has been inserted at the lowest level, it is possible that it is deleted
	// by a concurrent delete (e.g.DeleteMin) operation before it has been inserted at all levels.
	bool deleted = false;

	// Insert node to list at levels > 0.
	for (int i = 1; i < level; ++i) {
		new_node->valid_level = i;
		node1 = saved_nodes [i];
		Link::Lockable& anext = new_node->next [i];
		copy_node (new_node);
		for (BackOff bo; true; bo ()) {
			Node* node2 = scan_key (node1, i, new_node);
			anext = node2;
			release_node (node2);
			if (new_node->deleted) {
				deleted = true;
				anext = Link (nullptr, 1);
				release_node (new_node);
				release_node (node1);
				break;
			}
			if (node1->next [i].cas (node2, new_node)) {
				release_node (node1);
				break;
			}
		}

		if (deleted) {
			while (level > ++i)
				release_node (saved_nodes [i]);
			break;
		}
	}

	new_node->valid_level = (Level)level;
	if (deleted || new_node->deleted) {
		// Prevent real deletion.
		// The returned node will be removed from list, but existent.
		copy_node (new_node);
		release_node (help_delete (new_node, 0));
	}

	return new_node;
}

SkipListBase::Node* SkipListBase::read_node (Link::Lockable& node) noexcept
{
	Node* p = nullptr;
	Link link = node.lock ();
	if (!link.tag_bits () && (p = link))
		p->ref_cnt.increment ();
	node.unlock ();
	return p;
}

SkipListBase::Node* SkipListBase::read_next (Node*& node1, int level) noexcept
{
	if (node1->deleted)
		node1 = help_delete (node1, level);
	CHECK_VALID_LEVEL (level, node1);
	Node* node2 = read_node (node1->next [level]);
	while (!node2) {
		node1 = help_delete (node1, level);
		CHECK_VALID_LEVEL (level, node1);
		node2 = read_node (node1->next [level]);
	}
	return node2;
}

SkipListBase::Node* SkipListBase::scan_key (Node*& node1, int level, const Node* keynode) noexcept
{
	assert (keynode);
	Node* node2 = read_next (node1, level);
	while (less (*node2, *keynode)) {
		release_node (node1);
		node1 = node2;
		node2 = read_next (node1, level);
	}
	return node2;
}

SkipListBase::Node* SkipListBase::get_min_node () noexcept
{
	Node* prev = copy_node (head ());
	// Search the first node (node1) in the list that does not have
	// its deletion mark set.
	Node* node = read_next (prev, 0);
	if (node == tail ())
		node = nullptr;
	release_node (prev);
	return node;
}

SkipListBase::Node* SkipListBase::delete_min () noexcept
{
	// Start from the head node.
	Node* prev = copy_node (head ());
	Node* node1;

	for (;;) {
		// Search the first node (node1) in the list that does not have
		// its deletion mark set.
		node1 = read_next (prev, 0);
		if (node1 == tail ()) {
			release_node (prev);
			release_node (node1);
			return nullptr;
		}
	retry:
		if (node1 != prev->next [0].load ()) {
			release_node (node1);
			continue;
		}
		if (!node1->deleted) {
			// Try to set deletion mark.
			bool f = false;
			if (node1->deleted.compare_exchange_strong (f, true)) {
				// Succeeded, write valid pointer to the prev field of the node.
				// This prev field is necessary in order to increase the performance of concurrent
				// help_delete () functions, these operations otherwise
				// would have to search for the previous node in order to complete the deletion.
				node1->prev = prev;
				break;
			} else
				goto retry;
		} else
			node1 = help_delete (node1, 0);
		release_node (prev);
		prev = node1;
	}

	final_delete (node1);

	return node1;	// Reference counter have to be released by caller.
}

void SkipListBase::mark_next_links (Node* node, int from_level) noexcept
{
	// Mark the deletion bits of the next pointers of the node, starting with the from_level
	// level and going upwards.
	for (int i = from_level, end = node->level; i < end; ++i) {
		Link::Lockable& alink2 = node->next [i];
		Link node2;
		do {
			node2 = alink2.load ();
		} while (!(node2.tag_bits () || alink2.cas (node2, Link (node2, 1))));
	}
}

void SkipListBase::final_delete (Node* node) noexcept
{
	// Mark the deletion bits of the next pointers of the node, starting with the lowest
	// level and going upwards.
	mark_next_links (node, 0);

	// Actual deletion by calling the remove_node procedure, starting at the highest level
	// and continuing downwards. The reason for doing the deletion in decreasing order
	// of levels is that concurrent search operations also start at the
	// highest level and proceed downwards, in this way the concurrent
	// search operations will sooner avoid traversing this node.
	Node* prev = copy_node (head ());
	for (int i = node->level - 1; i >= 0; --i) {
		remove_node (node, prev, i);
	}
	release_node (prev);
}

/// Tries to fulfill the deletion on the current level and returns a reference to
/// the previous node when the deletion is completed.
SkipListBase::Node* SkipListBase::help_delete (Node* node, int level) noexcept
{
	assert (node != head ());
	assert (node != tail ());

	// Set the deletion mark on all next pointers in case they have not been set.
	mark_next_links (node, level);

	// Checks if the node given in the prev field is valid for deletion on the current level.
	Node* prev = node->prev;
	if (!prev || level >= prev->valid_level) {
		// Search for the correct previous node
		prev = copy_node (head ());
		for (int i = prev->level - 1; i >= level; --i) {
			Node* node2 = scan_key (prev, i, node);
			release_node (node2);
		}
	} else
		copy_node (prev);

	// The actual deletion of this node on the current level. 
	remove_node (node, prev, level);
	release_node (node);
	return prev;
}

/// Removes the given node from the linked list structure at the given level.
void SkipListBase::remove_node (Node* node, Node*& prev, int level) noexcept
{
	assert (node);
	assert (node != head ());
	assert (node != tail ());
	CHECK_VALID_LEVEL (level, node);

	// Address of next node pointer for the given level.
	Link::Lockable& anext = node->next [level];

	for (BackOff bo; true; bo ()) {

		// Synchronize with the possible other invocations to avoid redundant executions.
		if (anext.load () == Link (nullptr, 1))
			break;

		// Search for the right position of the previous node according to the key of node.
		Node* last = scan_key (prev, level, node);
		release_node (last);

		// Verify that node is still part of the linked list structure at the present level.
		if (last != node)
			break;

		Link next = anext.load ();
		// Synchronize with the possible other invocations to avoid redundant executions.
		if (next == Link (nullptr, 1))
			break;

		// Try to remove node by changing the next pointer of the previous node.
		if (prev->next [level].cas (node, next.untagged ())) {
			anext = Link (nullptr, 1);
			release_node (node);
			break;
		}

		// Synchronize with the possible other invocations to avoid redundant executions.
		if (anext.load () == Link (nullptr, 1))
			break;
	}
}

SkipListBase::Node* SkipListBase::lower_bound (const Node* keynode) noexcept
{
	assert (keynode);
	assert (keynode != head ());
	assert (keynode != tail ());

	Node* prev = copy_node (head ());
	for (unsigned i = this->max_level () - 1; i >= 1; --i) {
		Node* node2 = scan_key (prev, i, keynode);
		release_node (node2);
	}

	Node* node2 = scan_key (prev, 0, keynode);
	release_node (prev);

	if (node2 == tail ()) {
		release_node (node2);
		node2 = nullptr;
	}

	return node2;
}

SkipListBase::Node* SkipListBase::upper_bound (const Node* keynode) noexcept
{
	Node* node2 = lower_bound (keynode);
	if (node2) {
		if (!less (*keynode, *node2)) {
			Node* prev = node2;
			node2 = read_next (prev, 0);
			release_node (prev);
			if (node2 == tail ()) {
				release_node (node2);
				node2 = nullptr;
			}
		}
	}

	return node2;
}

SkipListBase::Node* SkipListBase::find (const Node* keynode) noexcept
{
	Node* node2 = lower_bound (keynode);
	if (node2) {
		if (less (*keynode, *node2)) {
			release_node (node2);
			node2 = nullptr;
		}
	}
	return node2;
}

SkipListBase::Node* SkipListBase::find_and_delete (const Node* keynode) noexcept
{
	assert (keynode);
	assert (keynode != head ());
	assert (keynode != tail ());

	Node* prev = copy_node (head ());
	for (unsigned i = max_level () - 1; i >= 1; --i) {
		Node* node2 = scan_key (prev, i, keynode);
		release_node (node2);
	}

	Node* node2 = scan_key (prev, 0, keynode);
	if (!less (*keynode, *node2)) {
		bool f = false;
		if (node2->deleted.compare_exchange_strong (f, true)) {
			// Succeeded, write valid pointer to the prev field of the node.
			// This prev field is necessary in order to increase the performance of concurrent
			// help_delete () functions, these operations otherwise
			// would have to search for the previous node in order to complete the deletion.
			node2->prev = prev;
			final_delete (node2);
			return node2;
		} else
			return nullptr; // Key found but was deleted in other thread.
	} else {
		release_node (prev);
		release_node (node2);
		return nullptr;
	}
}

bool SkipListBase::empty () noexcept
{
	Node* prev = copy_node (head ());
	Node* node = read_next (prev, 0);
	bool ret = node == tail ();
	release_node (prev);
	release_node (node);
	return ret;
}

}
}
