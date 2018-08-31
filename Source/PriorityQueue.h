// Nirvana project
// Lock-free priority queue based on the algorithm of Hakan Sundell and Philippas Tsigas.
// http://www.non-blocking.com/download/SunT03_PQueue_TR.pdf
// https://pdfs.semanticscholar.org/c9d0/c04f24e8c47324cb18ff12a39ff89999afc8.pdf

#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

#include <Nirvana.h>
#ifndef UNIT_TEST
#include "core.h"
#endif
#include "AtomicCounter.h"
#include "AtomicPtr.h"
#include "BackOff.h"
#include <random>
#include <algorithm>
#include <limits>

namespace Nirvana {
namespace Core {

/// Random-number generator.
typedef std::mt19937 RandomGen;

class _PriorityQueue
{
public:
	~_PriorityQueue ();

	/// Deletes node with minimal deadline.
	void* delete_min ();

	/// Gets minimal deadline.
	bool get_min_deadline (DeadlineTime& dt)
	{
		Node* prev = copy_node (head ());
		// Search the first node (node1) in the list that does not have
		// its deletion mark on the value set.
		Node* node = read_next (prev, 0);
		bool ret;
		if (node == tail ())
			ret = false;
		else {
			ret = true;
			dt = node->deadline;
		}
		release_node (prev);
		release_node (node);
		return ret;
	}

protected:
	struct Node;

	struct NodeBase
	{
		DeadlineTime deadline;
		AtomicCounter::UIntType timestamp;
		RefCounter ref_cnt;
		int level;
		int volatile valid_level;
		AtomicPtr <> value;
		Node* volatile prev;

		NodeBase (int l, DeadlineTime dt, void* v, AtomicCounter::UIntType ts) :
			deadline (dt),
			timestamp (ts),
			level (l),
			valid_level (1),
			value (v),
			prev (0)
		{}
	};

	typedef TaggedPtrT <Node, 1 << log2_ceil (sizeof (NodeBase))> Link;

	struct Node : public NodeBase
	{
		Link::Atomic next [1];	// Variable length array.

		Node (int l, DeadlineTime dt, void* v, AtomicCounter::UIntType ts) :
			NodeBase (l, dt, v, ts)
		{
			std::fill_n ((uintptr_t*)next, level, 0);
		}

		void* operator new (size_t cb, int level)
		{
#ifdef UNIT_TEST
			return _aligned_malloc (sizeof (Node) + (level - 1) * sizeof (next [0]), Link::alignment);
#else
			return g_core_heap->allocate (0, sizeof (Node) + (level - 1) * sizeof (next [0]), 0);
#endif
		}

		void operator delete (void* p, int level)
		{
#ifdef UNIT_TEST
			_aligned_free (p);
#else
			g_core_heap->release (p, sizeof (Node) + (level - 1) * sizeof (next [0]));
#endif
		}
	};

	bool less (const Node& n1, const Node& n2) const
	{
		if (n1.deadline < n2.deadline)
			return true;
		else if (n1.deadline > n2.deadline)
			return false;
		
		if (&n1 == head ())
			return &n2 != head ();
		else if (&n2 == head ())
			return false;

		if (&n2 == tail ())
			return &n1 != tail ();
		else if (&n1 == tail ())
			return false;

		AtomicCounter::UIntType current_ts = last_timestamp_;
		return current_ts - n1.timestamp > current_ts - n2.timestamp;
	}

	_PriorityQueue ();

	Node* head () const
	{
		return head_;
	}

	Node* tail () const
	{
		return tail_;
	}

	Node* create_node (int level, DeadlineTime dt, void* value)
	{
#ifdef _DEBUG
		node_cnt_.increment ();
#endif
		return new (level) Node (level, dt, value, last_timestamp_.increment ());
	}

	void delete_node (Node* node)
	{
		assert (node != head ());
		assert (node != tail ());
#ifdef _DEBUG
		for (int i = 0, end = node->level; i < end; ++i)
			assert (!(Node*)node->next [i].load ());
		assert (node_cnt_ > 0);
		node_cnt_.decrement ();
#endif
		Node::operator delete (node, node->level);
	}

	Node* read_node (Link::Atomic& node);

	static Node* copy_node (Node* node)
	{
		assert (node);
		node->ref_cnt.increment ();
		return node;
	}

	void release_node (Node* node);

	Node* read_next (Node*& node1, int level);

	Node* scan_key (Node*& node1, int level, Node* keynode);

	Node* help_delete (Node*, int level);

	void remove_node (Node* node, Node*& prev, int level);

protected:
	Node* head_;
	Node* tail_;
	const std::geometric_distribution <> distr_;
  static AtomicCounter last_timestamp_;
#ifdef _DEBUG
	AtomicCounter node_cnt_;
#endif
};

template <unsigned MAX_LEVEL>
class PriorityQueue :
	public _PriorityQueue
{
public:
	PriorityQueue ()
	{
		head_ = new (MAX_LEVEL) Node (MAX_LEVEL, 0, 0, 0);
		try {
			tail_ = new (1) Node (1, std::numeric_limits <DeadlineTime>::max (), 0, 0);
		} catch (...) {
			Node::operator delete (head_, head_->level);
			throw;
		}
		std::fill_n (head_->next, MAX_LEVEL, tail_);
		head_->valid_level = MAX_LEVEL;
	}

	/// Inserts new node.
	void insert (DeadlineTime dt, void* value, RandomGen& rndgen)
	{
		// Choose level randomly.
		int level = random_level (rndgen);

		// Create new node.
		Node* new_node = create_node (level, dt, value);
		copy_node (new_node);

		// Search phase to find the node after which newNode shouldbe inserted.
		// This search phase starts from the headnod e at the highest
		// level and traverses down to the lowest level until the correct
		// node is found (node1).When going down one level, the last
		// node traversed on that level is remembered (savedNodes)
		// for later use (this is where we shouldinsert the new node at that level).
		Node* node1 = copy_node (head ());
		Node* saved_nodes [MAX_LEVEL];
		for (int i = MAX_LEVEL - 1; i >= 1; --i) {
			Node* node2 = scan_key (node1, i, new_node);
			release_node (node2);
			if (i < level)
				saved_nodes [i] = copy_node (node1);
		}

		for (BackOff bo; true; bo.sleep ()) {
			Node* node2 = scan_key (node1, 0, new_node);
			/* This code was removed from the original algorithm because of keys can't
			be equal in this implementation.
			void* value2 = node2->value;
			if (!is_marked (value2) && node2->key == key) {
				if (cas (&node2->value, value2, value)) {
					release_node (node1);
					release_node (node2);
					for (int i = 1; i < level; ++i)
						release_node (saved_nodes [i]);
					release_node (new_node);
					release_node (new_node);
					return true;
				} else {
					release_node (node2);
					continue;
				}
			}
			*/

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
			Link::Atomic& anext = new_node->next [i];
			copy_node (new_node);
			for (BackOff bo; true; bo.sleep ()) {
				Node* node2 = scan_key (node1, i, new_node);
				anext = node2;
				release_node (node2);
				if (new_node->value.load ().is_marked ()) {
					deleted = true;
					anext = TaggedNil::marked ();
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

		new_node->valid_level = level;
		if (deleted || new_node->value.load ().is_marked ())
			new_node = help_delete (new_node, 0);
		release_node (new_node);
	}

private:
	int random_level (RandomGen& rndgen) const
	{
		return MAX_LEVEL > 1 ? std::min ((unsigned)(1 + distr_ (rndgen)), MAX_LEVEL) : 1;
	}
};

template <class T, unsigned MAX_LEVEL>
class PriorityQueueT : public PriorityQueue <MAX_LEVEL>
{
public:
	void insert (DeadlineTime dt, T* value, RandomGen& rndgen)
	{
		PriorityQueue::insert (dt, value, rndgen);
	}

	T* delete_min ()
	{
		return (T*)PriorityQueue::delete_min ();
	}
};

}
}

#endif
