#include "SkipList.h"

namespace Nirvana {
namespace Core {

using namespace std;

#ifdef _DEBUG
#define CHECK_VALID_LEVEL(lev, nod) assert(lev < nod->level)
#else
#define CHECK_VALID_LEVEL(lev, nod)
#endif

bool SkipListBase::Node::operator < (const Node&) const
{
	// Must be overridden.
	// Base method must never be called.
	assert (false);
	return false;
}

bool SkipListBase::less (const Node& n1, const Node& n2) const
{
	if (&n1 == &n2)
		return false;

	if (head () == &n1 || tail () == &n2)
		return true;

	if (tail () == &n1 || head () == &n2)
		return false;

	return n1 < n2; // Call virtual comparator
}

SkipListBase::SkipListBase (unsigned max_level) :
#ifdef _DEBUG
	node_cnt_ (0),
#endif
	distr_ (0.5)
{
	head_ = new (max_level) Node (max_level);
	try {
		tail_ = new (1) Node (1);
	} catch (...) {
		Node::operator delete (head_, head_->level);
		throw;
	}
	std::fill_n (head_->next, max_level, tail_);
	head_->valid_level = max_level;
}

SkipListBase::~SkipListBase ()
{
#ifdef _DEBUG
	assert (node_cnt_ == 0);
#endif
	Node::operator delete (head_, head_->level);
	Node::operator delete (tail_, tail_->level);
}

void SkipListBase::release_node (Node* node)
{
	assert (node);
	if (!node->ref_cnt.decrement ()) {
		Node* prev = node->prev;
		if (prev)
			release_node (prev);
		delete_node (node);
	}
}

void SkipListBase::delete_node (Node* node)
{
#ifdef _DEBUG
	assert (node != head ());
	assert (node != tail ());
	for (int i = 0, end = node->level; i < end; ++i)
		assert (!(Node*)node->next [i].load ());
	assert (node_cnt_ > 0);
	node_cnt_.decrement ();
#endif
	node->~Node ();
	Node::operator delete (node, node->level);
}

SkipListBase::Node* SkipListBase::read_node (Link::Lockable& node)
{
	Node* p = nullptr;
	Link link = node.lock ();
	if (!link.tag_bits ()) {
		if (p = link)
			p->ref_cnt.increment ();
	}
	node.unlock ();
	return p;
}

SkipListBase::Node* SkipListBase::read_next (Node*& node1, int level)
{
	if (node1->deleted)
		node1 = help_delete (node1, level);
	CHECK_VALID_LEVEL (level, node1);
	Node* node2 = read_node (node1->next [level]);
	while (!node2) {
		node1 = help_delete (node1, level);
		CHECK_VALID_LEVEL (level, node1);
		node2 = read_node (node1->next [level]);
	}
	return node2;
}

SkipListBase::Node* SkipListBase::scan_key (Node*& node1, int level, Node* keynode)
{
	assert (keynode);
	Node* node2 = read_next (node1, level);
	while (less (*node2, *keynode)) {
		release_node (node1);
		node1 = node2;
		node2 = read_next (node1, level);
	}
	return node2;
}

unsigned SkipListBase::random_level ()
{
	return 1 + distr_ (rndgen_);
}

SkipListBase::Node* SkipListBase::get_min_node ()
{
	Node* prev = copy_node (head ());
	// Search the first node (node1) in the list that does not have
	// its deletion mark set.
	Node* node = read_next (prev, 0);
	if (node == tail ())
		node = nullptr;
	release_node (prev);
	return node;
}

SkipListBase::Node* SkipListBase::delete_min ()
{
	// Start from the head node.
	Node* prev = copy_node (head ());
	Node* node1;

	for (;;) {
		// Search the first node (node1) in the list that does not have
		// its deletion mark set.
		node1 = read_next (prev, 0);
		if (node1 == tail ()) {
			release_node (prev);
			release_node (node1);
			return nullptr;
		}
	retry:
		if (node1 != prev->next [0].load ()) {
			release_node (node1);
			continue;
		}
		if (!node1->deleted) {
			// Try to set deletion mark.
			bool f = false;
			if (node1->deleted.compare_exchange_strong (f, true)) {
				// Succeeded, write valid pointer to the prev field of the node.
				// This prev field is necessary in order to increase the performance of concurrent
				// help_delete () functions, these operations otherwise
				// would have to search for the previous node in order to complete the deletion.
				node1->prev = prev;
				break;
			} else
				goto retry;
		} else
			node1 = help_delete (node1, 0);
		release_node (prev);
		prev = node1;
	}

	final_delete (node1);

	return node1;	// Reference counter have to be released by caller.
}

void SkipListBase::final_delete (Node* node)
{
	// Mark the deletion bits of the next pointers of the node, starting with the lowest
	// level and going upwards.
	for (int i = 0, end = node->level; i < end; ++i) {
		Link::Lockable& alink2 = node->next [i];
		Link node2;
		do {
			node2 = alink2.load ();
		} while ((Node*)node2 && !(
			node2.tag_bits ()
			||
			alink2.cas (node2, Link (node2, 1))
			));
	}

	// Actual deletion by calling the remove_node procedure, starting at the highest level
	// and continuing downwards. The reason for doing the deletion in decreasing order
	// of levels is that concurrent search operations also start at the
	// highest level and proceed downwards, in this way the concurrent
	// search operations will sooner avoid traversing this node.
	Node* prev = copy_node (head ());
	for (int i = node->level - 1; i >= 0; --i) {
		remove_node (node, prev, i);
	}
	release_node (prev);
}

/// Tries to fulfill the deletion on the current level and returns a reference to
/// the previous node when the deletion is completed.
SkipListBase::Node* SkipListBase::help_delete (Node* node, int level)
{
	assert (node != head ());
	assert (node != tail ());

	// Set the deletion mark on all next pointers in case they have not been set.
	for (int i = level, end = node->level; i < end; ++i) {
		Link::Lockable& alink2 = node->next [i];
		Link node2;
		do {
			node2 = alink2.load ();
		} while ((Node*)node2 && !(
			node2.tag_bits ()
			||
			alink2.cas (node2, Link (node2, 1))
			));
	}

	// Checks if the node given in the prev field is valid for deletion on the current level.
	Node* prev = node->prev;
	if (!prev || level >= prev->valid_level) {
		// Search for the correct previous node
		prev = copy_node (head ());
		for (int i = prev->level - 1; i >= level; --i) {
			Node* node2 = scan_key (prev, i, node);
			release_node (node2);
		}
	} else
		copy_node (prev);

	// The actual deletion of this node on the current level. 
	remove_node (node, prev, level);
	release_node (node);
	return prev;
}

/// Removes the given node from the linked list structure at the given level.
void SkipListBase::remove_node (Node* node, Node*& prev, int level)
{
	assert (node);
	assert (node != head ());
	assert (node != tail ());
	CHECK_VALID_LEVEL (level, node);

	// Address of next node pointer for the given level.
	Link::Lockable& anext = node->next [level];

	for (BackOff bo; true; bo ()) {

		// Synchronize with the possible other invocations to avoid redundant executions.
		if (anext.load () == Link (nullptr, 1))
			break;

		// Search for the right position of the previous node according to the key of node.
		Node* last = scan_key (prev, level, node);
		release_node (last);

		// Verify that node is still part of the linked list structure at the present level.
		if (last != node)
			break;

		Link next = anext.load ();
		// Synchronize with the possible other invocations to avoid redundant executions.
		if (next == Link (nullptr, 1))
			break;

		// Try to remove node by changing the next pointer of the previous node.
		if (prev->next [level].cas (node, next.untagged ())) {
			anext = Link (nullptr, 1);
			release_node (node);
			break;
		}

		// Synchronize with the possible other invocations to avoid redundant executions.
		if (anext.load () == Link (nullptr, 1))
			break;
	}
}

bool SkipListBase::erase (Node* node, unsigned max_level)
{
	assert (node);
	assert (node != head ());
	assert (node != tail ());

	Node* prev = copy_node (head ());
	for (unsigned i = max_level - 1; i >= 1; --i) {
		Node* node2 = scan_key (prev, i, node);
		release_node (node2);
	}

	for (;;) {
		Node* node2 = scan_key (prev, 0, node);
		if (!less (*node, *node2)) {
			bool f = false;
			if (node2->deleted.compare_exchange_strong (f, true)) {
				// Succeeded, write valid pointer to the prev field of the node.
				// This prev field is necessary in order to increase the performance of concurrent
				// help_delete () functions, these operations otherwise
				// would have to search for the previous node in order to complete the deletion.
				node2->prev = prev;
				final_delete (node2);
				release_node (node2);
				return true;
			}
		} else {
			release_node (prev);
			release_node (node2);
			return false;
		}
		release_node (prev);
		prev = node2;
	}
}

}
}