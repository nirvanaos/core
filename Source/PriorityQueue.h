// Nirvana project
// Lock-free priority queue based on the algorithm of Hakan Sundell and Philippas Tsigas.
// http://www.non-blocking.com/download/SunT03_PQueue_TR.pdf
// https://pdfs.semanticscholar.org/c9d0/c04f24e8c47324cb18ff12a39ff89999afc8.pdf

#ifndef NIRVANA_CORE_PRIORITYQUEUE_H_
#define NIRVANA_CORE_PRIORITYQUEUE_H_

//#define OLD_PRIORITY_QUEUE

#ifndef OLD_PRIORITY_QUEUE

#include "SkipList.h"

namespace Nirvana {
namespace Core {

template <class Val>
struct PriorityQueueKeyVal
{
	DeadlineTime deadline;
	Val val;

	bool operator < (const PriorityQueueKeyVal& rhs) const
	{
		if (deadline < rhs.deadline)
			return true;
		else if (deadline > rhs.deadline)
			return false;
		else
			return val < rhs.val;
	}
};

template <typename Val, unsigned MAX_LEVEL>
class PriorityQueue :
	public SkipList <PriorityQueueKeyVal <Val>, MAX_LEVEL>
{
	typedef SkipList <PriorityQueueKeyVal <Val>, MAX_LEVEL> Base;
	typedef Base::NodeVal NodeVal;
public:
	bool get_min_deadline (DeadlineTime& dt)
	{
		NodeVal* node = Base::get_min_node ();
		if (node) {
			dt = node->value ().deadline;
			Base::release_node (node);
			return true;
		}
		return false;
	}

	/// Inserts new node.
	bool insert (DeadlineTime dt, const Val& value)
	{
		return Base::insert ({ dt, value });
	}

	/// Deletes node with minimal deadline.
	/// \param [out] val Value.
	/// \return `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val)
	{
		NodeVal* node = Base::delete_min ();
		if (node) {
			val = node->value ().val;
			Base::release_node (node);
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
		NodeVal* node = Base::delete_min ();
		if (node) {
			deadline = node->value ().deadline;
			val = node->value ().val;
			Base::release_node (node);
			return true;
		}
		return false;
	}

	bool erase (DeadlineTime dt, const Val& value)
	{
		return Base::erase ({ dt, value });
	}
};

}
}

#else

#include "core.h"
#include "AtomicCounter.h"
#include "TaggedPtr.h"
#include "BackOff.h"
#include "RandomGen.h"
#include <algorithm>
#include <random>

namespace Nirvana {
namespace Core {

class PriorityQueueBase
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
	PriorityQueueBase (unsigned max_level, size_t node_size);
	~PriorityQueueBase ();

	struct Node;

	struct NodeBase
	{
		DeadlineTime deadline;
		Node* volatile prev;
		RefCounter ref_cnt;
		int level;
		int volatile valid_level;
		std::atomic <bool> deleted;

		NodeBase (int l, DeadlineTime dt) :
			deadline (dt),
			level (l),
			valid_level (1),
			deleted (false),
			prev (nullptr)
		{}

		virtual ~NodeBase ()
		{}
	};

	typedef TaggedPtrT <Node, 1, 1 << log2_ceil (sizeof (NodeBase))> Link;

	struct Node : public NodeBase
	{
		virtual bool operator < (const Node&) const;

		Link::Lockable next [1];	// Variable length array.

		Node (int level, DeadlineTime dt) :
			NodeBase (level, dt)
		{
			std::fill_n ((uintptr_t*)next, level, 0);
		}

		static size_t size (size_t cb, unsigned level)
		{
			return cb + (level - 1) * sizeof (next [0]);
		}

		static size_t size (unsigned level)
		{
			return size (sizeof (Node), level);
		}
	};

	/// Deletes node with minimal deadline.
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

	void release_node (Node* node);

	Node* read_next (Node*& node1, int level);

	Node* scan_key (Node*& node1, int level, Node* keynode);

	Node* help_delete (Node*, int level);

	unsigned random_level ();

	bool erase (Node* node, unsigned max_level);

private:
	void remove_node (Node* node, Node*& prev, int level);
	void delete_node (Node* node);
	void final_delete (Node* node);

protected:
#ifdef _DEBUG
	AtomicCounter node_cnt_;
#endif

private:
	const size_t node_size_;
	Node* head_;
	Node* tail_;
	RandomGenAtomic rndgen_;

	// This constant can be made static in future
	const std::geometric_distribution <> distr_;
};

template <unsigned MAX_LEVEL>
class PriorityQueueL :
	public PriorityQueueBase
{
protected:
	PriorityQueueL (size_t node_size) :
		PriorityQueueBase (MAX_LEVEL, node_size)
	{}

	unsigned random_level ()
	{
		return MAX_LEVEL > 1 ? std::min (PriorityQueueBase::random_level (), MAX_LEVEL) : 1;
	}

	bool insert (Node* new_node);

	bool erase (Node* node)
	{
		return PriorityQueueBase::erase (node, MAX_LEVEL);
	}
};

template <unsigned MAX_LEVEL>
bool PriorityQueueL <MAX_LEVEL>::insert (Node* new_node)
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

template <typename Val, unsigned MAX_LEVEL>
class PriorityQueue :
	public PriorityQueueL <MAX_LEVEL>
{
public:
	PriorityQueue () :
		PriorityQueueL <MAX_LEVEL> (sizeof (NodeVal))
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
		NodeVal node (1, dt, value);
		return PriorityQueueL <MAX_LEVEL>::erase (&node);
	}

private:
	struct NodeVal :
		public PriorityQueueBase::Node
	{
		// Reserve space for creation of auto variables with level = 1.
		uint8_t val_space [sizeof (Val)];

		Val& value ()
		{
			return *(Val*)((uint8_t*)this + PriorityQueueBase::Node::size (level));
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
		PriorityQueueBase::Node* p = (PriorityQueueBase::Node*)g_core_heap->allocate (0, PriorityQueueBase::Node::size (sizeof (NodeVal), level), 0);
		new (p) NodeVal (level, dt, value);
		return p;
	}
};

}
}
#endif
#endif
