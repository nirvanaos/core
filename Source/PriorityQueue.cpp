#include "PriorityQueue.h"
#include "BackOff.h"
#include <limits>

namespace Nirvana {
namespace Core {

using namespace std;

//#ifdef _DEBUG
//#define CHECK_VALID_LEVEL(lev, nod) assert(lev < nod->valid_level)
//#else
#define CHECK_VALID_LEVEL(lev, nod)
//#endif

inline PriorityQueue::Node::Node (int l, Key k, void* v) :
	key (k),
	level (l),
	valid_level (0),
	value (v),
	prev (0)
{
	fill_n ((uintptr_t*)next, level, 0);
}

PriorityQueue::PriorityQueue (unsigned max_level) :
	max_level_ (max (1u, min (MAX_LEVEL_MAX, max_level))),
#ifdef _DEBUG
	node_cnt_ (0),
#endif
	distr_ (0.5)
{
	head_ = new (max_level_) Node (max_level_, 0, 0);
	try {
		tail_ = new (max_level_) Node (max_level_, numeric_limits <Key>::max (), 0);
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
	if (node && !node->ref_cnt.decrement ()) {
		Node* prev = node->prev;
		if (prev)
			release_node (prev);
		delete_node (node);
	}
}

PriorityQueue::Node* PriorityQueue::read_node (AtomicLink <Node>& node)
{
	node.lock ();
	Link <Node> link = node.load ();
	Node* p = link;
	if (p)
		p->ref_cnt.increment ();
	node.unlock ();
	if (p && link.is_marked ()) {
		//release_node (p);
		p = nullptr;
	}
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

PriorityQueue::Node* PriorityQueue::scan_key (Node*& node1, int level, Key key)
{
	Node* node2 = read_next (node1, level);
	while (node2->key <= key) {
		release_node (node1);
		node1 = node2;
		node2 = read_next (node1, level);
	}
	return node2;
}

PriorityQueue::Node* PriorityQueue::scan_key (Node*& node1, int level, Node* keynode)
{
	assert (keynode);
	Node* node2 = read_next (node1, level);
	Key key = keynode->key;
	while (node2->key <= key && node2 != keynode) {
		release_node (node1);
		node1 = node2;
		node2 = read_next (node1, level);
	}
	return node2;
}

void PriorityQueue::insert (Key key, void* value, RandomGen& rndgen)
{
	int level = random_level (rndgen);
	Node* new_node = create_node (level, key, value);
	copy_node (new_node);
	Node* node1 = copy_node (head ());

	Node* saved_nodes [MAX_LEVEL_MAX];
	for (int i = max_level_ - 1; i >= 1; --i) {
		Node* node2 = scan_key (node1, i, key);
		release_node (node2);
		if (i < level)
			saved_nodes [i] = copy_node (node1);
	}

	for (BackOff bo; true; bo.sleep ()) {
		Node* node2 = scan_key (node1, 0, key);
		/*
		void* value2 = node2->value;
		if (!is_marked (value2) && node2->key == key) {
			if (cas (&node2->value, value2, value)) {
				release_node (node1);
				release_node (node2);
				for (int i = 1; i < level; ++i)
					release_node (saved_nodes [i]);
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
		node1 = saved_nodes [i];
		for (BackOff bo; true; bo.sleep ()) {
			Node* node2 = scan_key (node1, i, key);
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

void PriorityQueue::remove_node (Node* node, Node*& prev, int level)
{
	CHECK_VALID_LEVEL (level, node);
	for (BackOff bo; true; bo.sleep ()) {
		if (node->next [level].load () == Link <Node> (nullptr).marked ())
			break;
		Node* last = scan_key (prev, level, node);
		release_node (last);
		if (last != node)
			break;
		Link <Node> next = node->next [level].load ();
		if (next == Link <Node> (nullptr).marked ())
			break;
		if (prev->next [level].cas (node, next.unmarked ())) {
			node->next [level] = Link <Node> (nullptr).marked ();
			break;
		}
		if (node->next [level].load () == Link <Node> (nullptr).marked ())
			break;
	}
}

void* PriorityQueue::delete_min ()
{
	Node* prev = copy_node (head ());
	Node* node1;
	Link <void> value;

	for (;;) {
		node1 = read_next (prev, 0);
		if (node1 == tail ()) {
			release_node (prev);
			release_node (node1);
			return nullptr;
		}
	retry:
		value = node1->value.load ();
		if (node1 != prev->next [0].load ()) {
			release_node (node1);
			continue;
		}
		if (!value.is_marked ()) {
			if (node1->value.cas (value, value.marked ())) {
				node1->prev = prev;
				break;
			} else
				goto retry;
		} else //if (value.is_marked ())
			node1 = help_delete (node1, 0);

		release_node (prev);
		prev = node1;
	}

	for (int i = 0, end = node1->level; i < end; ++i) {
		Link <Node> node2;
		do {
			node2 = node1->next [i].load ();
		} while (!(
			node2.is_marked ()
			||
			node1->next [i].cas (node2, node2.marked ())
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

PriorityQueue::Node* PriorityQueue::help_delete (Node* node, int level)
{
	for (unsigned i = level, end = node->level; i < end; ++i) {
		Link <Node> node2;
		do {
			node2 = node->next [i].load ();
		} while (!(
			node2.is_marked ()
			||
			node->next [i].cas (node2, node2.marked ())
		));
	}
	
	Node* prev = node->prev;
	if (!prev || level >= prev->valid_level) {
		prev = copy_node (head ());
		for (int i = max_level_ - 1; i >= level; --i) {
			Node* node2 = scan_key (prev, i, node);
			release_node (node2);
		}
	} else
		copy_node (prev);

	remove_node (node, prev, level);
	release_node (node);
	return prev;
}

}
}