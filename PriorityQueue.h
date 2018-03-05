// Nirvana project
// Priority queue.
// Fibonacci Heap based.

#ifndef _PRIORITY_QUEUE_H_
#define _PRIORITY_QUEUE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "core.h"

class _PriorityQueue
{
public:

  class Item
  {
  public:
    
    Item (ExecutionTime deadline)
      : m_deadline (deadline)
    {}
    
    ExecutionTime deadline () const
    {
      return m_deadline;
    }
    
  private:
    
    void link (Item& child);
    void cut (Item& top);
    
    // Denied members
    Item (const Item&);
    Item& operator = (const Item&);
    
  private:
    
    friend class _PriorityQueue;
    
    ExecutionTime m_deadline;
    
    Item* m_left;  // left and right siblings in circular list
    Item* m_right;
    Item* m_parent;
    Item* m_children;
    
    UWord m_rank; // children count
    bool  m_mark;
  };  
  
  _PriorityQueue ()
    : m_first (0)
  {}

  void insert (Item& item);

  void remove_first ();

  Item* first () const
  {
    return m_first;
  }

  void decrease (Item& item, ExecutionTime deadline);

private:

  Item* m_first; // pointer to the list of roots
};

template <class T = _PriorityQueue::Item>
class PriorityQueue : public _PriorityQueue
{
public:

  void insert (T& item)
  {
    _PriorityQueue::insert (static_cast <Item&> (item));
  }

  T* first () const
  {
    return static_cast <T*> (_PriorityQueue::first ());
  }
};

#endif
