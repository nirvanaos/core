// Nirvana project
// Priority queue implementation

#include "PriorityQueue.h"
#include <memory.h>

void _PriorityQueue::Item::link (_PriorityQueue::Item& child)
{
  // Remove from current list
  child.m_left->m_right = child.m_right;
  child.m_right->m_left = child.m_left;

  // Append to children list
  if (m_children) {
    child.m_left = m_children;
    child.m_right = m_children->m_right;
    m_children->m_right->m_left = &child;
    m_children->m_right = &child;
  } else {
    m_children = &child;
    child.m_right = child.m_left = &child;
  }

  child.m_parent = this;
  ++m_rank;
  child.m_mark = false;
}

void _PriorityQueue::Item::cut (_PriorityQueue::Item& top)
{
  assert (m_parent->m_rank >= 1);
  if (m_parent->m_rank == 1)
    m_parent->m_children = 0;
  else {
    if (m_parent->m_children == this)
      m_parent->m_children = m_right;
    m_left->m_right = m_right;
    m_right->m_left = m_left;
  }
  --(m_parent->m_rank);
  m_parent = 0;
}

void _PriorityQueue::insert (Item& item)
{
  item.m_rank = 0;
  item.m_children = 0;
  item.m_parent = 0;
  item.m_mark = false;

  if (!m_first) {
    m_first = &item;
    item.m_right = item.m_left = &item;
  } else {
    item.m_left = m_first;
    item.m_right = m_first->m_right;
    m_first->m_right->m_left = &item;
    m_first->m_right = &item;
    if (item.m_deadline < m_first->m_deadline)
      m_first = &item;
  }
}

void _PriorityQueue::remove_first ()
{
  assert (m_first);
  
  Item* child = m_first->m_children;

  if (child) {
    
    while (child->m_parent) {
      child->m_parent = 0;
      child = child->m_right;
    }

    Item* sibling = m_first->m_right;
    child->m_left->m_right = sibling;
    m_first->m_right = child;    
    sibling->m_left = child->m_left;
    child->m_left = m_first;

  } else if (m_first->m_right == m_first) {
    m_first = 0;
    return;
  }

  Item* rank_array [sizeof (Pointer) * 8];
  zero (rank_array, rank_array + sizeof (Pointer) * 8);

  Item* lauf = m_first->m_right;
  Item* new_first = lauf;

  while (lauf != m_first) {
    
    Item* r1 = lauf;
    UWord rank = r1->m_rank;
    lauf = lauf->m_right;
    Item* r2;

    while (r2 = rank_array [rank]) {
      rank_array [rank] = 0;
      if (r1->m_deadline <= r2->m_deadline)
        r1->link (*r2);
      else {
        r2->link (*r1);
        r1 = r2;
      }
      rank++;
    }

    assert (!r1->m_parent);

    rank_array [rank] = r1;

    if (r1->m_deadline <= new_first->m_deadline)
      new_first = r1;
  }

  m_first->m_left->m_right = m_first->m_right;
  m_first->m_right->m_left = m_first->m_left;

  assert (!new_first->m_parent);
  m_first = new_first;
}

void _PriorityQueue::decrease (Item& item, ExecutionTime deadline)
{
  assert (item.m_deadline > deadline);

  item.m_deadline = deadline;

  Item* parent = item.m_parent;

  if (parent && deadline < parent->m_deadline) {
    
    Item* pitem = &item;

    pitem->cut (*m_first);

    pitem = parent;
    while (pitem->m_mark && (parent = pitem->m_parent)) {
      pitem->cut (*m_first);
      pitem = parent;
    }

    pitem->m_mark = true;
  }

  if (deadline < m_first->m_deadline)
    m_first = &item;
}