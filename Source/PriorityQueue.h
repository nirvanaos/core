// Nirvana project
// Lock-free priority queue based on the algorithm of Hakan Sundell and Philippas Tsigas.
// http://www.non-blocking.com/download/SunT03_PQueue_TR.pdf
// https://pdfs.semanticscholar.org/c9d0/c04f24e8c47324cb18ff12a39ff89999afc8.pdf

#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

#include <Nirvana.h>
#include "core.h"
#include "AtomicCounter.h"
#include "AtomicPtr.h"
#include "BackOff.h"
#include "RandomGen.h"
#include <random>
#include <algorithm>
#include <atomic>

namespace Nirvana {
namespace Core {

class _PriorityQueue
{
public:
	/// Gets minimal deadline.
	bool get_min_deadline (DeadlineTime& dt)
	{
		Node* prev = copy_node (head ());
		// Search the first node (node1) in the list that does not have
		// its deletion mark set.
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
	_PriorityQueue (unsigned max_level);
	~_PriorityQueue ();

	struct Node;

	struct NodeBase
	{
		DeadlineTime deadline;
		Node* volatile prev;
		RefCounter ref_cnt;
		AtomicCounter::UIntType timestamp;
		int level;
		int volatile valid_level;
		std::atomic <bool> deleted;

		NodeBase (int l, DeadlineTime dt, AtomicCounter::UIntType ts) :
			deadline (dt),
			timestamp (ts),
			level (l),
			valid_level (1),
			deleted (false),
			prev (0)
		{}

		virtual ~NodeBase ()
		{}
	};

	typedef TaggedPtrT <Node, 1 << log2_ceil (sizeof (NodeBase))> Link;

	struct Node : public NodeBase
	{
		Link::Atomic next [1];	// Variable length array.

		Node (int level, DeadlineTime dt, AtomicCounter::UIntType timestamp) :
			NodeBase (level, dt, timestamp)
		{
			std::fill_n ((uintptr_t*)next, level, 0);
		}

		static size_t size (unsigned level)
		{
			return sizeof (Node) + (level - 1) * sizeof (next [0]);
		}

		void* operator new (size_t cb, int level)
		{
#ifdef UNIT_TEST
			return _aligned_malloc (size (level), Link::alignment);
#else
			return g_core_heap->allocate (0, size (level), 0);
#endif
		}

		void operator delete (void* p, int level)
		{
#ifdef UNIT_TEST
			_aligned_free (p);
#else
			g_core_heap->release (p, size (level));
#endif
		}
	};

	/// Deletes node with minimal deadline.
	Node* delete_min ();

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

		AtomicCounter::IntType ts_diff = n1.timestamp - n2.timestamp;
		if (ts_diff < 0)
			return true;
		else if (ts_diff > 0)
			return false;

		// Different nodes can not be equal, otherwise, the algorithm will not works.
		// It is very unlikely that two nodes with equal deadlines will have equal timestamps.
		// But even in this case, we have to provide inequality.
		return &n1 < &n2;
	}

	Node* head () const
	{
		return head_;
	}

	Node* tail () const
	{
		return tail_;
	}

	void delete_node (Node* node);

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
  static AtomicCounter last_timestamp_;	// Maybe not static.
#ifdef _DEBUG
	AtomicCounter node_cnt_;
#endif
};

template <typename Val, unsigned MAX_LEVEL>
class PriorityQueue :
	public _PriorityQueue
{
public:
	PriorityQueue () :
		_PriorityQueue (MAX_LEVEL)
	{}

	/// Inserts new node.
	void insert (DeadlineTime dt, const Val& value, RandomGen& rndgen)
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
				if (new_node->deleted) {
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
		if (deleted || new_node->deleted)
			new_node = help_delete (new_node, 0);
		release_node (new_node);
	}

	bool delete_min (Val& val)
	{
		NodeVal* node = static_cast <NodeVal*> (_PriorityQueue::delete_min ());
		if (node) {
			val = node->value ();
			release_node (node);
			return true;
		}
		return false;
	}

private:
	struct NodeVal :
		public _PriorityQueue::Node
	{
		static size_t size (unsigned level)
		{
			return _PriorityQueue::Node::size (level) + sizeof (Val);
		}

		void* operator new (size_t cb, int level)
		{
#ifdef UNIT_TEST
			return _aligned_malloc (size (level), Link::alignment);
#else
			return g_core_heap->allocate (0, size (level), 0);
#endif
		}

		void operator delete (void* p, int level)
		{
#ifdef UNIT_TEST
			_aligned_free (p);
#else
			g_core_heap->release (p, size (level));
#endif
		}

		Val& value ()
		{
			return *(Val*)((::CORBA::Octet*)this + _PriorityQueue::Node::size (level));
		}

		NodeVal (int level, DeadlineTime deadline, AtomicCounter::UIntType timestamp, const Val& val) :
			_PriorityQueue::Node (level, deadline, timestamp)
		{
			new (&value ()) Val (val);
		}

		~NodeVal ()
		{
			value ().~Val ();
		}
	};

	Node* create_node (int level, DeadlineTime dt, const Val& value)
	{
#ifdef _DEBUG
		node_cnt_.increment ();
#endif
		return new (level) NodeVal (level, dt, last_timestamp_.increment (), value);
	}

	int random_level (RandomGen& rndgen) const
	{
		return MAX_LEVEL > 1 ? std::min ((unsigned)(1 + distr_ (rndgen)), MAX_LEVEL) : 1;
	}
};

}
}

#endif
