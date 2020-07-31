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

/// Base class implementing lock-free skip list algorithm.
class SkipListBase
{
public:
	bool empty () NIRVANA_NOEXCEPT
	{
		Node* prev = copy_node (head ());
		Node* node = read_next (prev, 0);
		bool ret = node == tail ();
		release_node (prev);
		release_node (node);
		return ret;
	}

protected:
	struct Node;

	SkipListBase (unsigned node_size, unsigned max_level, void* head_tail) NIRVANA_NOEXCEPT;
	~SkipListBase () NIRVANA_NOEXCEPT;

	struct NodeBase
	{
		Node* volatile prev;
		RefCounter ref_cnt;
		int level;
		int volatile valid_level;
		std::atomic <bool> deleted;

		NodeBase (int l) NIRVANA_NOEXCEPT :
			level (l),
			valid_level (1),
			deleted (false),
			prev (nullptr)
		{}

		virtual ~NodeBase () NIRVANA_NOEXCEPT
		{}
	};

	// Assume that key and value at least sizeof(void*) each.
	// So we add 3 * sizeof (void*) to the node size: next, key, and value.
	static const unsigned NODE_ALIGN = 1 << log2_ceil (sizeof (NodeBase) + 3 * sizeof (void*));
	typedef TaggedPtrT <Node, 1, NODE_ALIGN> Link;

	struct Node : public NodeBase
	{
		/// Virtual operator < must be overridden.
		virtual bool operator < (const Node&) const NIRVANA_NOEXCEPT;

		Link::Lockable next [1];	// Variable length array.

		Node (int level) NIRVANA_NOEXCEPT :
			NodeBase (level)
		{
			std::fill_n ((uintptr_t*)next, level, 0);
		}

		static constexpr size_t size (size_t node_size, unsigned level) NIRVANA_NOEXCEPT
		{
			return node_size + (level - 1) * sizeof (next [0]);
		}

		void* value () NIRVANA_NOEXCEPT
		{
			return next + level;
		}
	};

	/// Gets node with minimal key.
	/// \returns `Node*` if list not empty or `nullptr` otherwise.
	///          Returned Node* must be released by release_node().
	Node* get_min_node () NIRVANA_NOEXCEPT;

	/// Deletes node with minimal key.
	/// \returns `Node*` if list not empty or `nullptr` otherwise.
	///          Returned Node* must be released by release_node().
	Node* delete_min () NIRVANA_NOEXCEPT;

	// Node comparator.
	bool less (const Node& n1, const Node& n2) const NIRVANA_NOEXCEPT;

	Node* head () const NIRVANA_NOEXCEPT
	{
		return head_;
	}

	Node* tail () const NIRVANA_NOEXCEPT
	{
		return tail_;
	}

	unsigned max_level () const NIRVANA_NOEXCEPT
	{
		return head_->level;
	}

	Node* allocate_node (unsigned level);

	static Node* read_node (Link::Lockable& node) NIRVANA_NOEXCEPT;

	static Node* copy_node (Node* node) NIRVANA_NOEXCEPT
	{
		assert (node);
		node->ref_cnt.increment ();
		return node;
	}

	void release_node (Node* node) NIRVANA_NOEXCEPT;

	Node* read_next (Node*& node1, int level) NIRVANA_NOEXCEPT;

	Node* scan_key (Node*& node1, int level, Node* keynode) NIRVANA_NOEXCEPT;

	Node* help_delete (Node*, int level) NIRVANA_NOEXCEPT;

	unsigned random_level () NIRVANA_NOEXCEPT;

	bool erase (Node* node) NIRVANA_NOEXCEPT;

private:
	void remove_node (Node* node, Node*& prev, int level) NIRVANA_NOEXCEPT;
	void delete_node (Node* node) NIRVANA_NOEXCEPT;
	void final_delete (Node* node) NIRVANA_NOEXCEPT;

protected:
#ifdef _DEBUG
	AtomicCounter node_cnt_;
#endif

private:
	Node* head_;
	Node* tail_;
	unsigned node_size_;
	RandomGenAtomic rndgen_;

	// constant can be made static in future
	const std::geometric_distribution <> distr_;
};

/// Skip list implementation for the given maximal level count.
/// \tparam MAX_LEVEL Maximal level count.
template <unsigned MAX_LEVEL>
class SkipListL :
	public SkipListBase
{
protected:
	SkipListL (unsigned node_size) NIRVANA_NOEXCEPT :
		SkipListBase (node_size, MAX_LEVEL, head_tail_)
	{}

	bool insert (Node* new_node) NIRVANA_NOEXCEPT;

	unsigned random_level () NIRVANA_NOEXCEPT
	{
		return MAX_LEVEL > 1 ? std::min (SkipListBase::random_level (), MAX_LEVEL) : 1;
	}

private:
	// Memory space for head and tail nodes.
	// Add extra space for tail node alignment.
	uint8_t head_tail_ [Node::size (sizeof (Node), MAX_LEVEL) + Node::size (sizeof (Node), 1) + NODE_ALIGN - 1];
};

template <unsigned MAX_LEVEL>
bool SkipListL <MAX_LEVEL>::insert (Node* new_node) NIRVANA_NOEXCEPT
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

/// Skip list implementation for the given node value type and maximal level count.
/// \tparam Val Node value type. Must have operator <.
/// \tparam MAX_LEVEL Maximal level count.
template <typename Val, unsigned MAX_LEVEL>
class SkipList :
	public SkipListL <MAX_LEVEL>
{
	typedef SkipListL <MAX_LEVEL> Base;
	typedef Base::Node Node;
public:
	SkipList () NIRVANA_NOEXCEPT :
		Base (sizeof (Node) + sizeof (Val))
	{}

	/// Inserts new node.
	/// \param val Node value.
	/// \returns `true` if new node was inserted. `false` if node already exists.
	bool insert (const Val& val)
	{
		// Choose level randomly.
		unsigned level = Base::random_level ();

		// Allocate new node.
		Node* p = Base::allocate_node (level);

		// Initialize data
		new (p) NodeVal (level, val);

		// Insert node into skip list.
		return Base::insert (p);
	}

	/// Gets minimal value.
	/// \param [out] val Node value.
	/// \returns `true` if node found, `false` if queue is empty.
	bool get_min_value (Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal* node = get_min_node ();
		if (node) {
			val = node->value ();
			release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with minimal value.
	/// \param [out] val Node value.
	/// \returns `true` if node deleted, `false` if queue is empty.
	bool delete_min (Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal* node = delete_min ();
		if (node) {
			val = std::move (node->value ());
			release_node (node);
			return true;
		}
		return false;
	}

	/// Deletes node with specified value.
	/// \param val Node value to delete.
	/// \returns `true` if node deleted, `false` if value not found.
	bool erase (const Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal node (1, val);
		return Base::erase (&node);
	}

protected:
	struct NodeVal :
		public Node
	{
		// Reserve space for creation of auto variables with level = 1.
		uint8_t val_space [sizeof (Val)];

		Val& value () NIRVANA_NOEXCEPT
		{
			return *(Val*)Node::value ();
		}

		const Val& value () const NIRVANA_NOEXCEPT
		{
			return const_cast <NodeVal*> (this)->value ();
		}

		virtual bool operator < (const Node& rhs) const NIRVANA_NOEXCEPT
		{
			return value () < static_cast <const NodeVal&> (rhs).value ();
		}

		NodeVal (int level, const Val& val) NIRVANA_NOEXCEPT :
			Base::Node (level)
		{
			new (&value ()) Val (val);
		}

		~NodeVal () NIRVANA_NOEXCEPT
		{
			value ().~Val ();
		}
	};

	NodeVal* get_min_node () NIRVANA_NOEXCEPT
	{
		return static_cast <NodeVal*> (Base::get_min_node ());
	}

	NodeVal* delete_min () NIRVANA_NOEXCEPT
	{
		return static_cast <NodeVal*> (Base::delete_min ());
	}

	void release_node (NodeVal* node) NIRVANA_NOEXCEPT
	{
		Base::release_node (node);
	}
};

}
}

#endif
