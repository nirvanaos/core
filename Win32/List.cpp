// Nirvana project
// General list.

#include "List.h"

void _List::Item::cat (Item& next)
{
  next.m_prev->m_next = m_next;
  m_next->m_prev = next.m_prev;
  m_next = &next;
  next.m_prev = this;
}

void _List::Item::remove ()
{
  m_next->m_prev = m_prev;
  m_prev->m_next = m_next;
  m_next = m_prev = this;
}