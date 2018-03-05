#ifndef _LINE_REGIONS_H_
#define _LINE_REGIONS_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinMemory.h"

class WinMemory::LineRegions
{
public:
  
  class Region
  {
  public:
    UShort offset, size;
  };
  
  typedef const Region* Iterator;
  
  LineRegions ()
  {
    m_end = m_regions;
  }
  
  void push_back (UWord offset, UWord size);
  
  Iterator begin () const
  {
    return m_regions;
  }
  
  Iterator end () const
  {
    return m_end;
  }
  
  bool not_empty () const
  {
    return m_end > m_regions;
  }
  
  protected:
    Region  m_regions [10];
    Region* m_end;
};

#endif  // _LINE_REGIONS_H_
