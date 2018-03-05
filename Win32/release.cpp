#include "MemInfo.h"

void WinMemory::release (Pointer src, ULong size)
{
  if (!src)
    return;

  if (!size)
    return;
  
  Line* begin = Line::begin (src);
  Line* end = Line::end ((Octet*)src + size);
  check_allocated (begin->pages, end->pages);
  
  release (begin, end);
}

void WinMemory::release (Line* begin, Line* end)
{
  // Release full lines
  for (Line* line = begin; line < end;) {
    
    MemoryBasicInformation mbi (line);
    
    switch (mbi.Type) {
      
    case MEM_PRIVATE: // Reserved space
      {
        // Release entire region
        VirtualFree (mbi.AllocationBase, 0, MEM_RELEASE);
      
        // Re-reserve at begin
        UWord re_reserve = (UWord)line - (UWord)mbi.AllocationBase;
        if (re_reserve)
          verify (VirtualAlloc (mbi.AllocationBase, re_reserve, MEM_RESERVE, mbi.Protect));
        
        // Re-reserve at end
        line = (Line*)((UWord)mbi.BaseAddress + mbi.RegionSize);
        if ((UWord)line > (UWord)end)
          verify (VirtualAlloc (end, (UWord)line - (UWord)end, MEM_RESERVE, mbi.Protect));
      }          
      break;
      
    case MEM_MAPPED:  // Mapped space
      unmap_and_release (line);
      ++line;
      break;
    }
  }
}
