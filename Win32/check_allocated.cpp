#include "MemInfo.h"

UWord WinMemory::check_allocated (Page* begin, Page* end)
{
  assert (begin);
  assert (end);
  assert (!((UWord)begin % PAGE_SIZE));
  assert (!((UWord)end % PAGE_SIZE));
  assert (begin < end);

  // Start address must be valid
  check_pointer (begin);
  
  Line* line = Line::begin (begin);
  Line* end_line = Line::end (end);

  UWord allocation_protect = 0;
  
  // Check windows memory state
  do {
    
    MemoryBasicInformation mbi (line);

    // Check allocation

    if (MEM_FREE == mbi.State)
      throw BAD_PARAM (); // Not allocated

    allocation_protect |= mbi.AllocationProtect;
    
    // Check mapping

    switch (mbi.Type) {
      
    case MEM_PRIVATE:
      
      if (MEM_RESERVE != mbi.State)
        throw BAD_PARAM (); // This pages not from memory service
      
      if (mbi.RegionSize % LINE_SIZE)
        throw BAD_PARAM (); // This pages not from memory service
      
      line = (Line*)mbi.end ();
      break;
      
    case MEM_MAPPED:
      
      if (!mapping (mbi.BaseAddress))
        throw BAD_PARAM (); // This pages not from memory service
      
      ++line;
      break;
      
    default:
      throw BAD_PARAM (); // This pages not from memory service
    }
  } while (line < end_line);

  return allocation_protect;
}