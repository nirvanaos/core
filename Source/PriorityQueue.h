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
namespace Core {

class PriorityQueue
{
public:
	static const unsigned MAX_LEVEL_MAX = 32;

	/// Key type (deadline).
	typedef ::CORBA::ULongLong Key;

	/// Random-number generator.
	typedef std::mt19937 RandomGen;

	PriorityQueue (unsigned max_level = 8);
	~PriorityQueue ();

	void insert (Key key, void* value, RandomGen& rndgen);
	void* delete_min ();

private:
	struct Node;

	struct NodeBase
	{
		Key key;
		RefCounter ref_cnt;
		int level;
		int volatile valid_level;
		AtomicPtr <> value;
		Node* volatile prev;

		NodeBase (int l, Key k, void* v) :
			key (k),
			level (l),
			valid_level (0),
			value (v),
			prev (0)
		{}
	};

	typedef AtomicPtrT <Node, 1 << log2_ceil (sizeof (NodeBase))> Link;

	struct Node : public NodeBase
	{
		Link next [1];	// Variable length array.

		Node (int l, Key k, void* v) :
			NodeBase (l, k, v)
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

	Node* head () const
	{
		return head_;
	}

	Node* tail () const
	{
		return tail_;
	}

	Node* create_node (int level, Key key, void* value)
	{
#ifdef _DEBUG
		node_cnt_.increment ();
#endif
		return new (level) Node (level, key, value);
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

	Node* read_node (Link& node);

	static Node* copy_node (Node* node)
	{
		assert (node);
		node->ref_cnt.increment ();
		return node;
	}

	void release_node (Node* node);

	Node* read_next (Node*& node1, int level);

	Node* scan_key (Node*& node1, int level, Key key);
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
#ifdef _DEBUG
	AtomicCounter node_cnt_;
#endif
};

}
}

#endif
