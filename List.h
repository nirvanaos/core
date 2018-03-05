// Nirvana project
// General list.

#ifndef _LIST_H_
#define _LIST_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <iterator>

class _List
{
public:

  class Item
  {
  public:

    ~Item ()
    {
      remove ();
    }

    Item ()
    {
      m_prev = m_next = this;
    }

    void remove ();

    void cat (Item& next);

    bool listed () const
    {
      return m_next != this;
    }

    Item* next () const
    {
      return m_next;
    }

    Item* prev () const
    {
      return m_prev;
    }

    void swap (Item& rhs)
    {
      Item* tmp = m_next;
      m_next = rhs.m_next;
      rhs.m_next = tmp;
      tmp = m_prev;
      m_prev = rhs.m_prev;
      rhs.m_prev = tmp;
    }

  private:
    Item* m_next, *m_prev;
  };

private:

  Item m_head;
};

template <class T = List::Item, class Tag = T>
class List
{
public:

  class Item : public _List::Item
  {
  public:
    
    T* next () const
    {
      return static_cast <T*> (_List::Item::next ());
    }
    
    T* prev () const
    {
      return static_cast <T*> (_List::Item::prev ());
    }
    
    void cat (T& next)
    {
      _List::Item::cat (static_cast <_List::Item&> (next));
    }
  };  

  typedef T value_type;
  typedef const T& const_reference;
  typedef T& reference;

  typedef std::iterator <std::bidirectional_iterator_tag, T> _It;

  class iterator;

  class const_iterator : public _It
  {
  public:

    const_iterator ()
    {}

    const_iterator (const T* p)
      : m_ptr (const_cast <T*> (p))
    {}

    bool operator == (const const_iterator& rhs) const
    {
      return m_ptr == rhs.m_ptr;
    }

    bool operator != (const const_iterator& rhs) const
    {
      return m_ptr != rhs.m_ptr;
    }

    const_reference operator * () const
    {
      return *m_ptr;
    }

    const T* operator -> () const
    {
      return m_ptr;
    }

    const_iterator& operator ++ ()
    {
      m_ptr = static_cast <const Item*> (m_ptr)->next ();
      return *this;
    }
    
    const_iterator operator ++ (int)
    {
      const_iterator tmp = *this;
      operator ++ ();
      return tmp;
    }
    
    const_iterator& operator -- ()
    {
      m_ptr = static_cast <const Item*> (m_ptr)->prev ();
      return *this;
    }
    
    const_iterator operator -- (int)
    {
      const_iterator tmp = *this;
      operator -- ();
      return tmp;
    }
    
  protected:
    T* m_ptr;
  };

  class iterator : public const_iterator
  {
  public:

    iterator () :
      const_iterator ()
    {}

    iterator (T* p) :
      const_iterator (p)
    {}

    reference operator * () const
    {
      return *m_ptr;
    }
    
    T* operator -> () const
    {
      return m_ptr;
    }
    
    iterator& operator ++ ()
    {
      m_ptr = static_cast <Item*> (m_ptr)->next ();
      return *this;
    }
    
    iterator operator ++ (int)
    {
      iterator tmp = *this;
      operator ++ ();
      return tmp;
    }
    
    iterator& operator -- ()
    {
      m_ptr = static_cast <Item*> (m_ptr)->prev ();
      return *this;
    }
    
    iterator operator -- (int)
    {
      iterator tmp = *this;
      operator -- ();
      return tmp;
    }      
  };

  const_iterator begin () const
  {
    return const_iterator (m_head.next ());
  }

  iterator begin ()
  {
    return iterator (m_head.next ());
  }

  const_iterator end () const
  {
    return const_iterator (static_cast <const T*> (&m_head));
  }

  iterator end ()
  {
    return iterator (static_cast <T*> (&m_head));
  }

  const_reference back () const
  {
    return *(end ()->prev ());
  }

  reference back ()
  {
    return *(end ()->prev ());
  }

  void clear ()
  {
    while (!empty ())
      delete (&*begin ());
  }

  bool empty () const
  {
    return !m_head.listed ();
  }

  void insert (iterator it, T& elem)
  {
    static_cast <Item&> (elem).cat (*it);
  }

  void insert (iterator it, List& lst)
  {
    if (!lst.empty ()) {
      T& l = *lst->begin;
      lst.remove ();
      l.cat (*it);
    }
  }

  void push_back (T& elem)
  {
    insert (end (), elem);
  }

  void push_front (T& elem)
  {
    insert (begin (), elem);
  }
    
  void push_back (List& lst)
  {
    insert (end (), lst);
  }
  
  void push_front (List& lst)
  {
    insert (begin (), lst);
  }
  
  void remove (iterator it)
  {
    it->remove ();
  }
  
  void swap (List& rhs)
  {
    m_head.swap (rhs.m_head);
  }
};

#endif
