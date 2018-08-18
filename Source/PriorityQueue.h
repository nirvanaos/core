// Nirvana project
// Lock-free priority queue based on the algorithm of Hakan Sundell and Philippas Tsigas.
// http://www.non-blocking.com/download/SunT03_PQueue_TR.pdf
// https://pdfs.semanticscholar.org/c9d0/c04f24e8c47324cb18ff12a39ff89999afc8.pdf

#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

#ifndef UNIT_TEST
#include "core.h"
#endif
#include "AtomicCounter.h"
#include "AtomicPtr.h"
#include <random>
#include <algorithm>

namespace Nirvana {

typedef ::CORBA::ULongLong DeadlineTime;

namespace Core {

class PriorityQueue
{
public:
	static const unsigned MAX_LEVEL_MAX = 32;

	/// Random-number generator.
	typedef std::mt19937 RandomGen;

	PriorityQueue (unsigned max_level = 8);
	~PriorityQueue ();

	void insert (DeadlineTime dt, void* value, RandomGen& rndgen);
	void* delete_min ();

private:
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
			return g_default_heap->allocate (0, sizeof (Node) + (level - 1) * sizeof (next [0]), 0);
#endif
		}

		void operator delete (void* p, int level)
		{
#ifdef UNIT_TEST
			_aligned_free (p);
#else
			g_default_heap->release (p, sizeof (Node) + (level - 1) * sizeof (next [0]));
#endif
		}
	};

	static bool less (const Node& n1, const Node& n2)
	{
		if (n1.deadline < n2.deadline)
			return true;
		else if (n1.deadline > n2.deadline)
			return false;
		AtomicCounter::UIntType current_ts = last_timestamp_;
		return current_ts - n1.timestamp > current_ts - n2.timestamp;
	}

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

	int random_level (RandomGen& rndgen) const
	{
		return max_level_ > 1 ? std::min (1 + distr_ (rndgen), max_level_) : 1;
	}

private:
	Node* head_;
	Node* tail_;
	const std::geometric_distribution <> distr_;
	const int max_level_;
  static AtomicCounter last_timestamp_;
#ifdef _DEBUG
	AtomicCounter node_cnt_;
#endif
};

}
}

#endif
