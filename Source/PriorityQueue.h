#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

#include "core.h"
#include <InterlockedCounter.h>
#include <random>
#include <atomic>

namespace Nirvana {
namespace Core {

using namespace ::CORBA;

class PriorityQueue
{
	static const int MAX_LEVEL_MAX = 32;

public:
	typedef ULongLong Key;

private:
	template <typename T>
	class Link
	{
	public:
		Link ()
		{}

		Link (T* p) :
			ptr_ (p)
		{
			assert (!((intptr_t)p & 1));
		}

		intptr_t is_marked () const
		{
			return (intptr_t)ptr_ & 1;
		}

		Link marked () const
		{
			Link link;
			link.ptr_ = (void*)((intptr_t)ptr_ | 1);
			return link;
		}

		Link unmarked () const
		{
			Link link;
			link.ptr_ = (void*)((intptr_t)ptr_ & ~(intptr_t)1);
			return link;
		}

		operator T* ()
		{
			return (T*)((intptr_t)ptr_ & ~(intptr_t)1);
		}

		bool cas (Link <T> _cur, Link <T> _new)
		{
			void* cur = _cur.ptr_;
			return std::atomic_compare_exchange_strong ((volatile std::atomic <void*>*)&ptr_, &cur, _new.ptr_);
		}

		bool operator == (const Link <T>& rhs) const
		{
			return ptr_ == rhs.ptr_;
		}

	private:
		void* ptr_;
	};

	struct Node
	{
		InterlockedCounter ref_cnt;
		Key key;
		int level, valid_level;
		Link <void> value;
		Node* prev;
		Link <Node> next [1];	// Variable length array.

		Node (int l, Key k, void* v);

		void* operator new (size_t cb, int level)
		{
			return g_default_heap->allocate (0, sizeof (Node) + (level - 1) * sizeof (next [0]), 0);
		}

		void operator delete (void* p, int level)
		{
			g_default_heap->release (p, sizeof (Node) + (level - 1) * sizeof (next [0]));
		}
	};

public:
	/// Random-number generator.
	typedef std::mt19937 RandomGen;

	PriorityQueue (unsigned max_levels = 8);
	~PriorityQueue ();

	void insert (Key key, void* value, RandomGen& rndgen);

	void* delete_min ();

private:
	Node* head ()
	{
		return head_;
	}

	Node* tail ()
	{
		return &tail_;
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

	static Node* read_node (Link <Node> node)
	{
		Node* p = node;
		if (p)
			p->ref_cnt.increment ();
		if (node.is_marked ())
			return 0;
		else
			return p;
	}

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
	const int max_level_;
	Node* head_;
	Node tail_;
	const std::geometric_distribution <> distr_;
#ifdef _DEBUG
	InterlockedCounter node_cnt_;
#endif
};

}
}

#endif

