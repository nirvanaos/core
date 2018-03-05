#include "MemInfo.h"

void WinMemory::decommit (Pointer dst, UWord size)
{
  if (!dst)
    throw BAD_PARAM ();
  
  // Align to pages
  
  Page* begin = Page::end (dst);
  Page* end = Page::begin ((Octet*)dst + size);
  
  if (begin >= end)
    return;

  // Check current state

  if (check_allocated (begin, end) & ~WIN_MASK_WRITE)
    throw BAD_PARAM ();

  decommit (begin, end);
}

void WinMemory::decommit (Page* begin, Page* end)
{
  Line* begin_line = Line::end (begin);
  Line* end_line = Line::begin (end);
  
  // Unmap full lines
  
  for (Line* line = begin_line; line < end_line; ++line)
    unmap (line);

  // Decommit partial lines
  UWord cb;
  if (cb = (begin_line->pages - begin) * PAGE_SIZE)
    decommit_surrogate (begin, cb);
  if (cb = (end - end_line->pages) * PAGE_SIZE)
    decommit_surrogate (end_line->pages, cb);
}