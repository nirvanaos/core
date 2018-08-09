#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

#ifndef UNIT_TEST
#include "core.h"
#endif
#include <AtomicCounter.h>
#include <random>

#define USE_INTRINSIC_ATOMIC

#if (defined USE_INTRINSIC_ATOMIC && defined _MSC_BUILD)
#include <intrin.h>
#else
#include <atomic>
#endif

namespace Nirvana {
namespace Core {

class PriorityQueue
{
	static const unsigned MAX_LEVEL_MAX = 32;

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

		AtomicLink (Link <T> link) :
			ptr_ (link.ptr_)
		{}

		Link <T> load () const
		{
			Link <T> link;
			link.ptr_ = ptr_;
			return link;
		}

		Link <T> operator = (Link <T> link)
		{
			ptr_ = link.ptr_;
			return link;
		}

		bool cas (const Link <T>& _cur, const Link <T>& _new)
		{
			// We don't need to load current value, so we can use more lite function than STL.
#if (defined USE_INTRINSIC_ATOMIC && defined _MSC_BUILD)
#ifdef _M_X64
			return _InterlockedCompareExchange64 ((__int64 volatile*)&ptr_, (__int64)_new.ptr_, (__int64)_cur.ptr_) == (__int64)_cur.ptr_;
#else
			return _InterlockedCompareExchange ((long volatile*)&ptr_, (long)_new.ptr_, (long)_cur.ptr_) == (long)_cur.ptr_;
#endif
#else
			void* cur = _cur.ptr_;
			return ptr_.compare_exchange_strong (cur, _new.ptr_);
#endif
		}

	private:
#if (defined USE_INTRINSIC_ATOMIC && defined _MSC_BUILD)
		void* volatile ptr_;
#else
		std::atomic <void*> ptr_;
#endif
	};

	struct Node
	{
		RefCounter ref_cnt;
		Key key;
		int level;
		volatile int valid_level;
		AtomicLink <void> value;
		Node* prev;
		AtomicLink <Node> next [1];	// Variable length array.

		Node (int l, Key k, void* v);

		void* operator new (size_t cb, int level)
		{
#ifdef UNIT_TEST
			return malloc (sizeof (Node) + (level - 1) * sizeof (next [0]));
#else
			return g_default_heap->allocate (0, sizeof (Node) + (level - 1) * sizeof (next [0]), 0);
#endif
		}

		void operator delete (void* p, int level)
		{
#ifdef UNIT_TEST
			free (p);
#else
			g_default_heap->release (p, sizeof (Node) + (level - 1) * sizeof (next [0]));
#endif
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
			p->ref_cnt.increment ();
			if (!link.is_marked ())
				return p;
		}
		return 0;
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
	AtomicCounter node_cnt_;
#endif
};

}
}

#undef USE_INTRINSIC_ATOMIC

#endif

