#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

#include "core.h"
#include <random>
#include <atomic>

namespace Nirvana {
namespace Core {

using namespace ::CORBA;

class PriorityQueue
{
	static const int MAX_LEVELS_MAX = 32;

public:
	typedef ULongLong Key;

private:
	struct Node
	{
		std::atomic_ulong ref_cnt;
		Key key;
		int level, valid_level;
		void* value;
		Node* prev;
		Node* next [1];	// Variable length array.

		Node (int l, Key k, void* v);

		void* operator new (size_t cb, int level)
		{
			return g_default_heap->allocate (0, sizeof (Node) + (level - 1) * sizeof (Node*), 0);
		}

		void operator delete (void* p, int level)
		{
			g_default_heap->release (p, sizeof (Node) + (level - 1) * sizeof (Node*));
		}
	};

public:
	typedef std::mt19937 RandomGen;

	PriorityQueue (unsigned max_levels = 8);
	~PriorityQueue ();

	bool insert (Key key, void* value, RandomGen& rndgen);

	void* delete_min ();

private:
	static bool is_marked (void* p)
	{
		return (intptr_t)p & 1 != 0;
	}

	template <typename T>
	static T* get_marked (T* p)
	{
		return (T*)((intptr_t)p | 1);
	}

	template <typename T>
	static T* get_unmarked (T* p)
	{
		return (T*)((intptr_t)p & ~(intptr_t)1);
	}

	Node* head ()
	{
		return head_;
	}

	Node* tail ()
	{
		return &tail_;
	}

	static Node* create_node (int level, Key key, void* value)
	{
		return new (level) Node (level, key, value);
	}

	void delete_node (Node* node)
	{
		assert (node != head ());
		assert (node != tail ());
		Node::operator delete (node, node->level);
	}

	static Node* read_node (Node* node)
	{
		if (node && !is_marked (node)) {
			++(node->ref_cnt);
			return node;
		}
		return 0;
	}

	static Node* copy_node (Node* node)
	{
		if (node)
			++(node->ref_cnt);
		return node;
	}

	void release_node (Node* node);

	Node* read_next (Node*& node1, int level);

	Node* scan_key (Node*& node1, int level, Key key);

	Node* help_delete (Node*, int level);

	int random_level (RandomGen& rndgen) const
	{
		return max_levels_ > 1 ? std::min (1 + distr_ (rndgen), max_levels_) : 1;
	}

	template <typename T>
	static bool cas (T** pp, T* pcur, T* pnew)
	{
		void* cur = pcur;
		return std::atomic_compare_exchange_strong ((volatile std::atomic <void*>*)pp, &cur, (void*)pnew);
	}

private:
	const int max_levels_;
	Node* head_;
	Node tail_;
	const std::geometric_distribution <> distr_;
};

}
}

#endif

