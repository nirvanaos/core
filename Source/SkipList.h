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

	void* allocate_node (unsigned level);

	bool insert (Node* new_node, Node** saved_nodes) NIRVANA_NOEXCEPT;

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

	Node* find (Node* keynode) NIRVANA_NOEXCEPT;
	Node* upper_bound (Node* keynode) NIRVANA_NOEXCEPT;
	
	/// Erase by key.
	bool erase (Node* keynode) NIRVANA_NOEXCEPT;

	/// Directly erases node.
	/// `erase_node (find (keynode));` works slower then `erase (keynode);`.
	bool erase_node (Node* node) NIRVANA_NOEXCEPT
	{
		bool f = false;
		if (node->deleted.compare_exchange_strong (f, true)) {
			final_delete (node);
			return true;
		}
		return false;
	}

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
	typedef SkipListBase Base;
protected:
	SkipListL (unsigned node_size) NIRVANA_NOEXCEPT :
		Base (node_size, MAX_LEVEL, head_tail_)
	{}

	bool insert (Node* new_node) NIRVANA_NOEXCEPT
	{
		Node* save_nodes [MAX_LEVEL];
		return Base::insert (new_node, save_nodes);
	}

	unsigned random_level () NIRVANA_NOEXCEPT
	{
		return MAX_LEVEL > 1 ? std::min (Base::random_level (), MAX_LEVEL) : 1;
	}

private:
	// Memory space for head and tail nodes.
	// Add extra space for tail node alignment.
	uint8_t head_tail_ [Node::size (sizeof (Node), MAX_LEVEL) + Node::size (sizeof (Node), 1) + NODE_ALIGN - 1];
};

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

		// Initialize node and insert it into skip list.
		return Base::insert (new (Base::allocate_node (level)) NodeVal (level, val));
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
		NodeVal keynode (1, val);
		return Base::erase (&keynode);
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

	NodeVal* find (const Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal keynode (1, val);
		return static_cast <NodeVal*> (Base::find (&keynode));
	}

	NodeVal* upper_bound (const Val& val) NIRVANA_NOEXCEPT
	{
		NodeVal keynode (1, val);
		return static_cast <NodeVal*> (Base::upper_bound (&keynode));
	}

	/// Directly erases node.
	/// `erase_node (find (key));` works slower then `erase (key);`.
	bool erase_node (NodeVal* node) NIRVANA_NOEXCEPT
	{
		return Base::erase_node (node);
	}
};

}
}

#endif
