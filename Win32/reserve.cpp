#include "MemInfo.h"

Line* WinMemory::reserve (UWord size, UWord flags, Line* dst)
{
  assert (size);
  assert (!((UWord)dst % LINE_SIZE));

  UWord protect;
  if (flags & READ_ONLY)
    protect = WIN_READ_RESERVE;
  else
    protect = WIN_WRITE_RESERVE;
  
  // Reserve full lines

  UWord full_lines_size = (size + LINE_SIZE - 1) & ~(LINE_SIZE - 1);
  Line* begin = (Line*)VirtualAlloc (dst, full_lines_size, MEM_RESERVE, protect);
  if (!(begin || dst))
    throw NO_MEMORY ();

  assert (!((UWord)begin % LINE_SIZE));

  return begin;
}
