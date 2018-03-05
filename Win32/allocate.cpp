#include "WinMemory.h"

Pointer WinMemory::allocate (Pointer dst, UWord size, UWord flags)
{
  if (!size)
    throw BAD_PARAM ();
  
  if ((READ_ONLY & flags) && !(RESERVED & flags))
    throw INV_FLAG ();
  
  // Allocation unit is one line (64K)
  Line* begin = Line::begin (dst);
  UWord offset = (Octet*)dst - begin->pages->bytes;
  if (offset)
    if (!(flags & EXACTLY)) {
      begin = 0;
      offset = 0;
    }

  if (begin) {

    if (!reserve (size + offset, flags, begin)) {
      
      if (flags & EXACTLY)
        return 0;

      begin = reserve (size, flags);
    }
    
  } else {

    if (flags & EXACTLY)
      throw BAD_PARAM ();

    begin = reserve (size, flags);
  }
  
  dst = begin->pages->bytes + offset;

  if (!(flags & RESERVED)) {

    Octet* end = (Octet*)dst + size;

    try {
      commit (Page::begin (dst), Page::end (end), flags & ZERO_INIT);
    } catch (...) {
      release (begin, Line::end (end));
      throw;
    }
  }
  
  return dst;
}

