#ifndef _MEM_INFO_H_
#define _MEM_INFO_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinMemory.h"

class WinMemory::MemoryBasicInformation : public MEMORY_BASIC_INFORMATION
{
public:

  MemoryBasicInformation (const void* p)
  {
    assert (p);
    verify (VirtualQuery (p, this, sizeof (MEMORY_BASIC_INFORMATION)));
  }

  MemoryBasicInformation ()
  {}

  Page* begin () const
  {
    return (Page*)BaseAddress;
  }

  Page* end () const
  {
    return (Page*)((UWord)BaseAddress + RegionSize);
  }

  Line* allocation_base () const
  {
    return (Line*)AllocationBase;
  }

  bool write_copied () const
  {
    return (WIN_WRITE_COPIED == Protect);
  }

  bool committed () const // Not same as MEM_COMMIT!
  {
    return (MEM_COMMIT == State) && (Protect > PAGE_NOACCESS);
  }

  bool operator ++ ()
  {
    verify (VirtualQuery (end (), this, sizeof (MEMORY_BASIC_INFORMATION)));
  }

  void at (const void* p)
  {
    verify (VirtualQuery (p, this, sizeof (MEMORY_BASIC_INFORMATION)));
  }
};

#endif  // _MEM_INFO_H_