#include "MemInfo.h"

HANDLE WinMemory::mapping (const void* p)
{
  try {
    return logical_line (p).m_mapping;
  } catch (...) {
    return 0;
  }
}

HANDLE WinMemory::new_mapping ()
{
  HANDLE h = CreateFileMapping (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
  if (!h)
    throw NO_MEMORY ();
  return h;
}

bool WinMemory::map (Line* line, HANDLE mapping)
{
  // Check for current stack intersects with line
  if (((void*)&mapping >= line) && ((void*)&mapping < (line + 1))) {
    // Call in separate stack
    MapParam param = { line, mapping };
    call_in_fiber ((FiberMethod)map_in_fiber, &param);
    return param.ret;
  }

  LogicalLine& ll = logical_line (line);
  if (!VirtualAlloc (&ll, sizeof (ll), MEM_COMMIT, PAGE_READWRITE))
    throw NO_MEMORY ();

  bool is_new_mapping;
  if (is_new_mapping = !mapping) {
    if (ll.m_mapping)
      return false;
    mapping = new_mapping ();
  }

  // No remapping possible for this object location.
  assert (((void*)(this + 1) <= line) || ((void*)this >= (line + 1)));
  
  try {

    MemoryBasicInformation mbi (line);

    assert (MEM_FREE != mbi.State);

    if (MEM_PRIVATE == mbi.Type) {
  
      if (MEM_RESERVE != mbi.State)
        throw BAD_PARAM ();

      assert (!ll.m_mapping);
  
      VirtualFree (mbi.AllocationBase, 0, MEM_RELEASE);
  
      UWord re_reserve = (UWord)line - (UWord)mbi.AllocationBase;
      if (re_reserve)
        verify (VirtualAlloc (mbi.AllocationBase, re_reserve, MEM_RESERVE, mbi.AllocationProtect));
    
      re_reserve = (UWord)mbi.BaseAddress + mbi.RegionSize - (UWord)(line + 1);
      if (re_reserve)
        verify (VirtualAlloc ((line + 1), re_reserve, MEM_RESERVE, mbi.AllocationProtect));
      
    } else {
  
      assert (mbi.AllocationBase == line);

      assert (ll.m_mapping);
  
      if (mapping != ll.m_mapping)
        unmap (line, ll);
      else if (!UnmapViewOfFile (line))
        throw BAD_PARAM ();
    }

    UWord map_access;
    if (WIN_MASK_WRITE & mbi.AllocationProtect)
      map_access = FILE_MAP_WRITE;
    else
      map_access = FILE_MAP_READ;

    if (!MapViewOfFileEx (mapping, map_access, 0, 0, LINE_SIZE, line))
      throw INTERNAL ();

  } catch (...) {
    if (is_new_mapping)
      CloseHandle (mapping);
    throw;
  }

  if (mapping != ll.m_mapping) {  // if mapping has replaced
    ll.m_mapping = mapping;
    // link in single list element
    ll.m_line_next = ll.m_line_prev = line_index (line);
    return true;
  }

  return false;
}

void WinMemory::map_in_fiber (MapParam* param)
{
  param->ret = map (param->line, param->mapping);
}

void WinMemory::map (Line* dst, const Line* src)
{
  UShort src_li = line_index (src);
  LogicalLine& src_ll = logical_line (src_li);

  assert (src_ll.m_mapping);

  bool is_new = map (dst, src_ll.m_mapping);

  // If source mapping is old then pages can be committed elsewhere.
  // In this case, memory state can be MEM_COMMIT, not MEM_RESERVE.
  // So, we must 'decommit' this pages now.
  
  if (!VirtualFree (dst, PAGES_PER_LINE, MEM_DECOMMIT)) {
    
    // MEM_DECOMMIT does not work for memory mappings
    
    Page* begin = dst->pages;
    Page* end = (dst + 1)->pages;
    while (begin < end) {
      
      MemoryBasicInformation mbi (begin);
      
      Page* reg_end = mbi.end ();
      if (reg_end > end)
        reg_end = end;
      
      if (MEM_COMMIT == mbi.State) {
        UWord old;
        UWord reg_size = (UWord)reg_end - (UWord)begin;
        verify (VirtualProtect (begin, reg_size, PAGE_NOACCESS, &old));
        verify (VirtualAlloc (begin, reg_size, MEM_RESET, PAGE_NOACCESS));
      }
      
      begin = reg_end;
    }
  }
  
  if (is_new) {

    // Link lines to cycle list.

    UShort dst_li = line_index (dst);
    LogicalLine& dst_ll = logical_line (dst_li);

    Lock lock (m_critical_section);

    LogicalLine& next_ll = logical_line (dst_ll.m_line_next = src_ll.m_line_next);
    next_ll.m_line_prev = dst_li;
    dst_ll.m_line_prev = src_li;
    src_ll.m_line_next = dst_li;
  }
}

void WinMemory::unmap (Line* line)
{
  MemoryBasicInformation mbi (line);
  
  assert (MEM_FREE != mbi.State);
 
  if (MEM_MAPPED == mbi.Type) {

    assert (mbi.AllocationBase == line);

    unmap_and_release (line);
    
    verify (VirtualAlloc (line, LINE_SIZE, MEM_RESERVE, 
      (mbi.AllocationProtect & WIN_MASK_READ) ? WIN_READ_RESERVE : WIN_WRITE_RESERVE));
  }
}

void WinMemory::unmap (Line* line, LogicalLine& ll)
{
  assert (ll.m_mapping);
  
  if (!UnmapViewOfFile (line))
    throw BAD_PARAM ();

  Lock lock (m_critical_section);
  
  if (ll.shared_next () == line)  // line not shared
    CloseHandle (ll.m_mapping);   // close mapping
  else {
    
    // Remove line from list.
    logical_line (ll.m_line_next).m_line_prev = ll.m_line_prev;
    logical_line (ll.m_line_prev).m_line_next = ll.m_line_next;
  }
  
  ll.m_mapping = 0;
  ll.m_line_next = ll.m_line_prev = 0;
}

HANDLE WinMemory::remap_line (Octet* exclude_begin, Octet* exclude_end, LineState& line_state, Word remap_type)
{
  assert (remap_type != REMAP_NONE);
    
  HANDLE file_mapping;
  if (REMAP_FULL <= remap_type)
    file_mapping = new_mapping ();
  else {
    file_mapping = mapping (exclude_begin);
    assert (file_mapping);
  }
    
  try {
    
    Line* tmp_line = (Line*)MapViewOfFileEx (file_mapping, FILE_MAP_WRITE, 0, 0, LINE_SIZE, 0);
    if (!tmp_line)
      throw NO_MEMORY ();
    Line* line = Line::begin (exclude_begin);
    Page* begin_page = Page::begin (exclude_begin);
    Page* tmp_page = tmp_line->pages;
    Octet* page_state_ptr = line_state.m_page_states;
    Line* line_end = line + 1;
    Page* page = line->pages;
    
    do {
      
      if (
        (REMAP_FULL == remap_type)
        ?
        (PAGE_COMMITTED & *page_state_ptr)
        :
        (PAGE_COPIED & *page_state_ptr)
      ) {

        assert ((REMAP_FULL == remap_type) || (PAGE_VIRTUAL_PRIVATE& *page_state_ptr));

        if ((exclude_end <= page->bytes) || (page < begin_page))
          *tmp_page = *page;
        else {
          Page* page_end = page + 1;
          if (page->bytes < exclude_begin)
            real_copy (page->bytes, exclude_begin, tmp_page->bytes);
          if (page_end->bytes > exclude_end) {
            UWord offset = exclude_end - page->bytes;
            real_copy (exclude_end, page_end->bytes, tmp_page->bytes + offset);
          }
        }
        
        // Modify page state
        *page_state_ptr = PAGE_MAPPED_PRIVATE;
      }
      
      ++tmp_page;
      ++page_state_ptr;
      ++page;
    } while (page < line_end->pages);
    
    map (line, file_mapping);
    UnmapViewOfFile (tmp_line);
    
    if (REMAP_PART == remap_type) {
      
      // ¬озможно, строка содержит decommitted страницы.
      // ѕосле map, эти страницы будут доступны.
      // »митируем decommit, запреща€ доступ к этим страницам.

      page_state_ptr = line_state.m_page_states;
      page = line->pages;
      do {
        
        if (PAGE_DECOMMITTED == (PAGE_COPIED & *page_state_ptr))
          if ((page < begin_page) || (exclude_end <= page->bytes))
            decommit_surrogate (page, PAGE_SIZE);
          
        ++page_state_ptr;
        ++page;
      } while (page < line_end->pages);
    }
    
  } catch (...) {
    
    if (REMAP_FULL == remap_type)
      CloseHandle (file_mapping);
    
    throw;
  }

  return file_mapping;
}
