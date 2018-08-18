#include "PriorityQueue.h"
#include "BackOff.h"
#include <limits>

// Doesn't work without this
#define WAIT_FULL_INSERT

namespace Nirvana {
namespace Core {

using namespace std;

#ifdef _DEBUG
#define CHECK_VALID_LEVEL(lev, nod) assert(lev < nod->level)
#else
#define CHECK_VALID_LEVEL(lev, nod)
#endif

AtomicCounter PriorityQueue::last_timestamp_ = 0;

PriorityQueue::PriorityQueue (unsigned max_level) :
	max_level_ (max (1u, min (MAX_LEVEL_MAX, max_level))),
#ifdef _DEBUG
	node_cnt_ (0),
#endif
	distr_ (0.5)
{
	head_ = new (max_level_) Node (max_level_, 0, 0, 0);
	try {
		tail_ = new (max_level_) Node (max_level_, numeric_limits <DeadlineTime>::max (), 0, 0);
	} catch (...) {
		Node::operator delete (head_, head_->level);
		throw;
	}
	fill_n (head_->next, max_level_, tail_);
	head_->valid_level = max_level_;
}

PriorityQueue::~PriorityQueue ()
{
#ifdef _DEBUG
	assert (node_cnt_ == 0);
#endif
	Node::operator delete (head_, head_->level);
	Node::operator delete (tail_, tail_->level);
}

void PriorityQueue::release_node (Node* node)
{
	assert (node);
	if (!node->ref_cnt.decrement ()) {
		Node* prev = node->prev;
		if (prev)
			release_node (prev);
		delete_node (node);
	}
}

PriorityQueue::Node* PriorityQueue::read_node (Link::Atomic& node)
{
	Node* p = nullptr;
	Link link = node.lock ();
	if (!link.is_marked ()) {
		if (p = link)
			p->ref_cnt.increment ();
	}
	node.unlock();
	return p;
}

PriorityQueue::Node* PriorityQueue::read_next (Node*& node1, int level)
{
	if (node1->value.load ().is_marked ())
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

PriorityQueue::Node* PriorityQueue::scan_key (Node*& node1, int level, Node* keynode)
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

void PriorityQueue::insert (DeadlineTime dt, void* value, RandomGen& rndgen)
{
	assert (dt > 0 && dt < numeric_limits <DeadlineTime>::max ());

	// Choose level randomly.
	int level = random_level (rndgen);
	
	// Create new node.
	Node* new_node = create_node (level, dt, value);
	copy_node (new_node);

	// Search phase to find the node after which newNode shouldbe inserted.
	// This search phase starts from the headnod e at the highest
	// level and traverses down to the lowest level until the correct
	// node is found (node1).When going down one level, the last
	// node traversed on that level is remembered (savedNodes)
	// for later use (this is where we shouldinsert the new node at that level).
	Node* node1 = copy_node (head ());
	Node* saved_nodes [MAX_LEVEL_MAX - 1];
	for (int i = max_level_ - 1; i >= 1; --i) {
		Node* node2 = scan_key (node1, i, new_node);
		release_node (node2);
		if (i < level)
			saved_nodes [i - 1] = copy_node (node1);
	}

	for (BackOff bo; true; bo.sleep ()) {
		Node* node2 = scan_key (node1, 0, new_node);
		/*
		void* value2 = node2->value;
		if (!is_marked (value2) && node2->key == key) {
			if (cas (&node2->value, value2, value)) {
				release_node (node1);
				release_node (node2);
				for (int i = 1; i < level; ++i)
					release_node (saved_nodes [i - 1]);
				release_node (new_node);
				release_node (new_node);
				return true;
			} else {
				release_node (node2);
				continue;
			}
		}
		*/
		new_node->next [0] = node2;
		release_node (node2);
		if (node1->next [0].cas (node2, new_node)) {
			release_node (node1);
			break;
		}
	}

	for (int i = 1; i < level; ++i) {
		new_node->valid_level = i;
		node1 = saved_nodes [i - 1];
		for (BackOff bo; true; bo.sleep ()) {
			Node* node2 = scan_key (node1, i, new_node);
			new_node->next [i] = node2;
			release_node (node2);
			if (new_node->value.load ().is_marked () || node1->next [i].cas (node2, new_node)) {
				release_node (node1);
				break;
			}
		}
	}

	new_node->valid_level = level;
	if (new_node->value.load ().is_marked ())
		new_node = help_delete (new_node, 0);
	release_node (new_node);
}

void* PriorityQueue::delete_min ()
{
	Node* prev = copy_node (head ());
	Node* node1;
	TaggedPtr <> value;

	for (;;) {
		node1 = read_next (prev, 0);
		if (node1 == tail ()) {
			release_node (prev);
			release_node (node1);
			return nullptr;
		}
#ifdef WAIT_FULL_INSERT
		if (node1->valid_level >= node1->level) {
#endif
		retry:
			if (node1 != prev->next [0].load ()) {
				release_node (node1);
				continue;
			}
			value = node1->value.load ();
			if (!value.is_marked ()) {
				if (node1->value.cas (value, value.marked ())) {
					node1->prev = prev;
					break;
				} else
					goto retry;
			} else //if (value.is_marked ())
				node1 = help_delete (node1, 0);
#ifdef WAIT_FULL_INSERT
		}
#endif
		release_node (prev);
		prev = node1;
	}

	for (int i = 0, end = node1->level; i < end; ++i) {
		Link::Atomic& alink2 = node1->next [i];
		Link node2;
		do {
			node2 = alink2.load ();
		} while (!(
			node2.is_marked ()
			||
			alink2.cas (node2, node2.marked ())
		));
	}
	prev = copy_node (head ());
	for (int i = node1->level - 1; i >= 0; --i) {
		remove_node (node1, prev, i);
	}
	release_node (prev);
	release_node (node1);
	release_node (node1);	// Delete node.
	return value;
}

/// Tries to fulfill the deletion on the current level and returns a reference to
/// the previous node when the deletion is completed.
PriorityQueue::Node* PriorityQueue::help_delete (Node* node, int level)
{
	assert (node != head ());
	assert (node != tail ());

	// Set the deletion mark on all next pointers in case they have not been set.
	for (unsigned i = level, end = node->level; i < end; ++i) {
		assert (end <= max_level_);
		Link::Atomic& alink2 = node->next [i];
		Link node2;
		do {
			node2 = alink2.load ();
		} while (!(
			node2.is_marked ()
			||
			alink2.cas (node2, node2.marked ())
		));
	}
	
	// Checks if the node given in the prev field is valid for deletion on the current level.
	Node* prev = node->prev;
	if (!prev || level >= prev->valid_level) {
		// Search for the correct previous node
		prev = copy_node (head ());
		for (int i = max_level_ - 1; i >= level; --i) {
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
void PriorityQueue::remove_node (Node* node, Node*& prev, int level)
{
	assert (node != head ());
	assert (node != tail ());
	CHECK_VALID_LEVEL (level, node);

	// Address of next node pointer for the given level.
	Link::Atomic& anext = node->next [level];

	for (BackOff bo; true; bo.sleep ()) {

		// Synchronize with the possible other invocations to avoid redundant executions.
		if (anext.load () == TaggedNil::marked ())
			break;
		
		// Search for the right position of the previous node according to the key of node.
		Node* last = scan_key (prev, level, node);
		release_node (last);
		
		// Verify that node is still part of the linked list structure at the present level.
		if (last != node)
			break;

		Link next = anext.load ();
		// Synchronize with the possible other invocations to avoid redundant executions.
		if (next == TaggedNil::marked ())
			break;
		
		// Try to remove node by changing the next pointer of the previous node.
		if (prev->next [level].cas (node, next.unmarked ())) {
			anext = TaggedNil::marked ();
			break;
		}
		
		// Synchronize with the possible other invocations to avoid redundant executions.
		if (anext.load () == TaggedNil::marked ())
			break;
	}
}

}
}