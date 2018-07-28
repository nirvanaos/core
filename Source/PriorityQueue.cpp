#include "PriorityQueue.h"
#include <limits>
#include <algorithm>

namespace Nirvana {
namespace Core {

using namespace std;

PriorityQueue::Node::Node (int l, Key k, void* v) :
	ref_cnt (1),
	key (k),
	level (l),
	valid_level (0),
	value (v),
	prev (0)
{
	zero (next, next + l);
}

PriorityQueue::PriorityQueue (unsigned max_levels) :
	max_levels_ (max_levels),
	distr_ (0.5),
	tail_ (1, numeric_limits <Key>::max (), 0)
{
	assert (max_levels_ > 0);
	assert (max_levels_ <= MAX_LEVELS_MAX);
	head_ = create_node (max_levels_, 0, 0);
	tail_.prev = head_;
	fill_n (head_->next, max_levels_, &tail_);
}

PriorityQueue::~PriorityQueue ()
{
	Node::operator delete (head_, head_->level);
}

void PriorityQueue::release_node (Node* node)
{
	node = get_unmarked (node);
	if (node && !--(node->ref_cnt)) {
		for (Node** pp = node->next, **end = pp + node->level; pp != end; ++pp)
			release_node (*pp);
		release_node (node->prev);
		delete_node (node);
	}
}

PriorityQueue::Node* PriorityQueue::read_next (Node*& node1, int level)
{
	if (is_marked (node1->value))
		node1 = help_delete (node1, level);
	assert (level < node1->level);
	Node* node2 = read_node (node1->next [level]);
	while (!node2) {
		node1 = help_delete (node1, level);
		node2 = read_node (node1->next [level]);
	}
	return node2;
}

PriorityQueue::Node* PriorityQueue::scan_key (Node*& node1, int level, Key key)
{
	Node* node2 = read_next (node1, level);
	while (node2->key < key) {
		release_node (node1);
		node1 = node2;
		node2 = read_next (node1, level);
	}
	return node2;
}

bool PriorityQueue::insert (Key key, void* value, RandomGen& rndgen)
{
	int level = random_level (rndgen);
	Node* new_node = create_node (level, key, value);
	copy_node (new_node);
	Node* node1 = copy_node (head ());

	Node* saved_nodes [MAX_LEVELS_MAX];
	for (int i = max_levels_ - 1; i >= 1; --i) {
		Node* node2 = scan_key (node1, i, key);
		release_node (node2);
		if (i < level)
			saved_nodes [i] = copy_node (node1);
	}

	for (;;) {
		Node* node2 = scan_key (node1, 0, key);
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

		new_node->next [0] = node2;
		release_node (node2);
		if (cas (&(node1->next [0]), node2, new_node)) {
			release_node (node1);
			break;
		}
	}

	for (int i = 1; i < level; ++i) {
		new_node->valid_level = i;
		node1 = saved_nodes [i];
		for (;;) {
			Node* node2 = scan_key (node1, i, key);
			new_node->next [i] = node2;
			release_node (node2);
			if (is_marked (new_node->value) || cas (&(node1->next [i]), node2, new_node)) {
				release_node (node1);
				break;
			}
		}
	}

	new_node->valid_level = level;
	if (is_marked (new_node->value))
		new_node = help_delete (new_node, 0);
	release_node (new_node);
	return true;
}

void* PriorityQueue::delete_min ()
{
	Node* prev = copy_node (head ());
	Node* node1;
	void* value;

	for (;;) {
		node1 = read_next (prev, 0);
		if (node1 == tail ()) {
			release_node (prev);
			release_node (node1);
			return 0;
		}
	retry:
		value = node1->value;
		if (!is_marked (value)) {
			if (cas (&node1->value, value, get_marked (value))) {
				node1->prev = prev;
				break;
			} else
				goto retry;
		} else //if (is_marked (value))
			node1 = help_delete (node1, 0);

		release_node (prev);
		prev = node1;
	}

	for (int i = 0, end = node1->level; i < end; ++i) {
		Node* node2;
		do {
			node2 = node1->next [i];
		} while (
			!is_marked (node2)
			&&
			!cas (&node1->next [i], node2, get_marked (node2))
		);
	}
	prev = copy_node (head ());
	for (int i = node1->level - 1; i >= 0; --i) {
		for (;;) {
			if (node1->next [i] == get_marked ((Node*)0))
				break;
			Node* last = scan_key (prev, i, node1->key);
			release_node (last);
			if (last != node1 || node1->next [i] == get_marked ((Node*)0))
				break;
			if (cas (&(prev->next [i]), node1, get_unmarked (node1->next [i]))) {
				node1->next [i] = get_marked ((Node*)0);
				break;
			}
			if (node1->next [i] == get_marked ((Node*)0))
				break;
		}
	}
	release_node (prev);
	release_node (node1);
	release_node (node1);	// Delete node.
	return value;
}

PriorityQueue::Node* PriorityQueue::help_delete (Node* node, int level)
{
	for (unsigned i = level, end = node->level; i < end; ++i) {
		Node* node2;
		do {
			node2 = node->next [i];
		} while (!is_marked (node2) && !cas (&(node->next [i]), node2, get_marked (node2)));
	}
	
	Node* prev = node->prev;
	if (!prev || level >= prev->valid_level) {
		prev = copy_node (head ());
		for (int i = max_levels_ - 1; i >= level; --i) {
			Node* node2 = scan_key (prev, i, node->key);
			release_node (node2);
		}
	} else
		copy_node (prev);

	for (;;) {
		if (node->next [level] == get_marked ((Node*)0))
			break;
		Node* last = scan_key (prev, level, node->key);
		release_node (last);
		if (last != node || node->next [level] == get_marked ((Node*)0))
			break;
		if (cas (&(prev->next [level]), node, get_unmarked (node->next [level]))) {
			node->next [level] = get_marked ((Node*)0);
			break;
		}
		if (node->next [level] == get_marked ((Node*)0))
			break;
	}
	release_node (node);
	return prev;
}

}
}