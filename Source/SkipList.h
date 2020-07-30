// Nirvana project
// Lock-free skip list based on the algorithm of Hakan Sundell and Philippas Tsigas.
// http://www.non-blocking.com/download/SunT03_PQueue_TR.pdf
// https://pdfs.semanticscholar.org/c9d0/c04f24e8c47324cb18ff12a39ff89999afc8.pdf

#ifndef NIRVANA_CORE_SKIPLIST_H_
#define NIRVANA_CORE_SKIPLIST_H_

#include "core.h"
#include "AtomicCounter.h"
#include "TaggedPtr.h"
#include "BackOff.h"
#include "RandomGen.h"
#include <algorithm>
#include <random>

namespace Nirvana {
namespace Core {

/// <summary>
/// Base class implementing lock-free skip list algorithm.
/// </summary>
class SkipListBase
{
public:
	bool empty ()
	{
		Node* prev = copy_node (head ());
		Node* node = read_next (prev, 0);
		bool ret = node == tail ();
		release_node (prev);
		release_node (node);
		return ret;
	}

protected:
	SkipListBase (unsigned max_level);
	~SkipListBase ();

	struct Node;

	struct NodeBase
	{
		Node* volatile prev;
		RefCounter ref_cnt;
		int level;
		int volatile valid_level;
		std::atomic <bool> deleted;

		NodeBase (int l) :
			level (l),
			valid_level (1),
			deleted (false),
			prev (nullptr)
		{}

		virtual ~NodeBase ()
		{}
	};

	/// <summary>
	/// TaggedPtr assumes that key and value at least sizeof(void*) each.
	/// </summary>
	typedef TaggedPtrT <Node, 1, 1 << log2_ceil (sizeof (NodeBase) + 3 * sizeof (void*))> Link;

	struct Node : public NodeBase
	{
		/// <summary>
		/// Virtual operator < must be overridden.
		/// </summary>
		virtual bool operator < (const Node&) const;

		Link::Lockable next [1];	// Variable length array.

		Node (int level) :
			NodeBase (level)
		{
			std::fill_n ((uintptr_t*)next, level, 0);
		}

		static size_t size (unsigned level)
		{
			return sizeof (Node) + (level - 1) * sizeof (next [0]);
		}

		void* operator new (size_t cb, int level)
		{
			return g_core_heap->allocate (0, size (level), 0);
		}

		void operator delete (void* p, int level)
		{
			g_core_heap->release (p, size (level));
		}
	};

	/// Gets node with minimal key.
	/// \returns `Node*` if list not empty or `nullptr` otherwise.
	///          Returned Node* must be released by release_node().
	Node* get_min_node ();

	/// Deletes node with minimal key.
	/// \returns `Node*` if list not empty or `nullptr` otherwise.
	///          Returned Node* must be released by release_node().
	Node* delete_min ();

	// Node comparator.
	bool less (const Node& n1, const Node& n2) const;

	Node* head () const
	{
		return head_;
	}

	Node* tail () const
	{
		return tail_;
	}

	static Node* read_node (Link::Lockable& node);

	static Node* copy_node (Node* node)
	{
		assert (node);
		node->ref_cnt.increment ();
		return node;
	}

#ifndef _DEBUG
	static
#endif
	void release_node (Node* node);

	Node* read_next (Node*& node1, int level);

	Node* scan_key (Node*& node1, int level, Node* keynode);

	Node* help_delete (Node*, int level);

	unsigned random_level ();

	bool erase (Node* node, unsigned max_level);

private:
	void remove_node (Node* node, Node*& prev, int level);
#ifndef _DEBUG
	static
#endif
	void delete_node (Node* node);
	void final_delete (Node* node);

protected:
#ifdef _DEBUG
	AtomicCounter node_cnt_;
#endif

private:
	Node* head_;
	Node* tail_;
	RandomGenAtomic rndgen_;

	// constant can be made static in future
	const std::geometric_distribution <> distr_;
};

/// <summary>
/// Skip list implementation for the given MAX_LEVEL value.
/// </summary>
template <unsigned MAX_LEVEL>
class SkipListL :
	public SkipListBase
{
protected:
	SkipListL () :
		SkipListBase (MAX_LEVEL)
	{}

	unsigned random_level ()
	{
		return MAX_LEVEL > 1 ? std::min (SkipListBase::random_level (), MAX_LEVEL) : 1;
	}

	bool insert (Node* new_node);

	bool erase (Node* node)
	{
		return SkipListBase::erase (node, MAX_LEVEL);
	}
};

template <unsigned MAX_LEVEL>
bool SkipListL <MAX_LEVEL>::insert (Node* new_node)
{
	int level = new_node->level;
	copy_node (new_node);

	// Search phase to find the node after which newNode should be inserted.
	// This search phase starts from the head node at the highest
	// level and traverses down to the lowest level until the correct
	// node is found (node1).When going down one level, the last
	// node traversed on that level is remembered (savedNodes)
	// for later use (this is where we should insert the new node at that level).
	Node* node1 = copy_node (head ());
	Node* saved_nodes [MAX_LEVEL];
	for (int i = MAX_LEVEL - 1; i >= 1; --i) {
		Node* node2 = scan_key (node1, i, new_node);
		release_node (node2);
		if (i < level)
			saved_nodes [i] = copy_node (node1);
	}

	for (BackOff bo; true; bo ()) {
		Node* node2 = scan_key (node1, 0, new_node);
		if (!node2->deleted && !less (*new_node, *node2)) {
			// The same value with the same deadline already exists.
			// This case should not occurs in the real operating, but we support it.
			release_node (node1);
			release_node (node2);
			for (int i = 1; i < level; ++i)
				release_node (saved_nodes [i]);
			release_node (new_node);
			release_node (new_node);
			return false;
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

	new_node->valid_level = level;
	if (deleted || new_node->deleted)
		new_node = help_delete (new_node, 0);
	release_node (new_node);

	return true;
}
/*
template <typename Val, unsigned MAX_LEVEL>
class PriorityQueue :
	public PriorityQueueL <MAX_LEVEL>
{
public:
	PriorityQueue ()
	{}

	/// Inserts new node.
	bool insert (DeadlineTime dt, const Val& value)
	{
		// Choose level randomly.
		int level = PriorityQueueL <MAX_LEVEL>::random_level ();

		// Create new node.
		return PriorityQueueL <MAX_LEVEL>::insert (create_node (level, dt, value));
	}

	/// Deletes node with minimal deadline.
	/// \param [out] val Value.
	/// \return `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val)
	{
		NodeVal* node = static_cast <NodeVal*> (PriorityQueueBase::delete_min ());
		if (node) {
			val = std::move (node->value ());
			PriorityQueueBase::release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with minimal deadline.
	/// \param [out] val Value.
	/// \param [out] deadline Deadline.
	/// \return `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val, DeadlineTime& deadline)
	{
		NodeVal* node = static_cast <NodeVal*> (PriorityQueueBase::delete_min ());
		if (node) {
			val = std::move (node->value ());
			deadline = node->deadline;
			PriorityQueueBase::release_node (node);
			return true;
		}
		return false;
	}

	bool erase (DeadlineTime dt, const Val& value)
	{
		PriorityQueueBase::Node* node = create_node (1, dt, value);
		bool ret = PriorityQueueL <MAX_LEVEL>::erase (node);
		PriorityQueueBase::release_node (node);
		return ret;
	}

private:
	struct NodeVal :
		public PriorityQueueBase::Node
	{
		static size_t size (unsigned level)
		{
			return PriorityQueueBase::Node::size (level) + sizeof (Val);
		}

		void* operator new (size_t cb, int level)
		{
			return g_core_heap->allocate (0, size (level), 0);
		}

		void operator delete (void* p, int level)
		{
			g_core_heap->release (p, size (level));
		}

		Val& value ()
		{
			return *(Val*)((::CORBA::Octet*)this + PriorityQueueBase::Node::size (level));
		}

		const Val& value () const
		{
			return const_cast <NodeVal*> (this)->value ();
		}

		virtual bool operator < (const PriorityQueueBase::Node& rhs) const
		{
			return value () < static_cast <const NodeVal&> (rhs).value ();
		}

		NodeVal (int level, DeadlineTime deadline, const Val& val) :
			PriorityQueueBase::Node (level, deadline)
		{
			new (&value ()) Val (val);
		}

		~NodeVal ()
		{
			value ().~Val ();
		}
	};

	PriorityQueueBase::Node* create_node (int level, DeadlineTime dt, const Val& value)
	{
#ifdef _DEBUG
		PriorityQueueBase::node_cnt_.increment ();
#endif
		return new (level) NodeVal (level, dt, value);
	}
};
*/
}
}

#endif
