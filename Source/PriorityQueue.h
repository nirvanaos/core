#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

#include "core.h"
#include <AtomicCounter.h>
#include <random>
#include <atomic>

namespace Nirvana {
namespace Core {

class PriorityQueue
{
	static const int MAX_LEVEL_MAX = 32;

public:
	typedef ::CORBA::ULongLong Key;

private:
	template <typename T> class AtomicLink;

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

		Link (const Link <T>& src) :
			ptr_ (src.ptr_)
		{}

		T* operator = (T* p)
		{
			assert (!((intptr_t)p & 1));
			ptr_ = p;
			return p;
		}

		Link& operator = (const Link <T>& src)
		{
			ptr_ = src.ptr_;
			return *this;
		}

		intptr_t is_marked () const
		{
			return (intptr_t)ptr_ & 1;
		}

		Link <T> marked () const
		{
			Link <T> link;
			link.ptr_ = (void*)((intptr_t)ptr_ | 1);
			return link;
		}

		Link <T> unmarked () const
		{
			Link <T> link;
			link.ptr_ = (void*)((intptr_t)ptr_ & ~(intptr_t)1);
			return link;
		}

		operator T* () const
		{
			return (T*)((intptr_t)ptr_ & ~(intptr_t)1);
		}

		bool operator == (const Link <T>& rhs) const
		{
			return ptr_ == rhs.ptr_;
		}

	private:
		friend class AtomicLink <T>;
		void* ptr_;
	};

	template <typename T>
	class AtomicLink
	{
	public:
		AtomicLink ()
		{}

		AtomicLink (T* p) :
			ptr_ (p)
		{
			assert (!((intptr_t)p & 1));
		}

		Link <T> load () const
		{
			Link <T> link;
			link.ptr_ = ptr_.load ();
			return link;
		}

		Link <T> operator = (Link <T> link)
		{
			ptr_ = link.ptr_;
			return link;
		}

		T* operator = (T* p)
		{
			assert (!((intptr_t)p & 1));
			ptr_ = p;
			return p;
		}

		bool cas (const Link <T>& _cur, const Link <T>& _new)
		{
			void* cur = _cur.ptr_;
			return ptr_.compare_exchange_strong (cur, _new.ptr_);
		}

	private:
		std::atomic <void*> ptr_;
	};

	struct Node
	{
		AtomicCounter ref_cnt;
		Key key;
		int level;
		std::atomic <int> valid_level;
		AtomicLink <void> value;
		Node* prev;
		AtomicLink <Node> next [1];	// Variable length array.

		Node (int l, Key k, void* v);

		void* operator new (size_t cb, int level)
		{
			//return malloc (sizeof (Node) + (level - 1) * sizeof (next [0]));
			return g_default_heap->allocate (0, sizeof (Node) + (level - 1) * sizeof (next [0]), 0);
		}

		void operator delete (void* p, int level)
		{
			//free (p);
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

	static Node* read_node (const AtomicLink <Node>& node)
	{
		Link <Node> link = node.load ();
		Node* p = link;
		if (p) {
			assert (p->ref_cnt > 0);
			p->ref_cnt.increment ();
			if (!link.is_marked ())
				return p;
		}
		return 0;
	}

	static Node* copy_node (Node* node)
	{
		assert (node);
		assert (node->ref_cnt > 0);
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
	AtomicCounter node_cnt_;
#endif
};

}
}

#endif

