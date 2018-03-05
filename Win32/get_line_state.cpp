#include "MemInfo.h"

void WinMemory::get_line_state (const Line* line, LineState& state)
{
  assert (!((UWord)line % LINE_SIZE));

  bool check_sharing = false;

  {
    MemoryBasicInformation mbi (line);

    if (MEM_FREE == mbi.State) {
      state.m_allocation_protect = 0;
      mbi.Type = 0;
    } else
      state.m_allocation_protect = mbi.AllocationProtect;

    if (MEM_MAPPED != mbi.Type) {
      zero ((UWord*)state.m_page_states, (UWord*)(state.m_page_states + PAGES_PER_LINE));
      return;
    }

    Octet* page_state_ptr = state.m_page_states;
    const Page* page = line->pages;
    const Line* line_end = line + 1;
    for (;;) {

      assert (MEM_FREE != mbi.State);
      assert (mbi.allocation_base () == line);

      Octet page_state;
      if (MEM_COMMIT == mbi.State) {
        
        if (PAGE_NOACCESS == mbi.Protect)
          page_state = PAGE_DECOMMITTED | PAGE_VIRTUAL_PRIVATE;
        else if (WIN_MASK_MAPPED & mbi.Protect)
          page_state = PAGE_MAPPED_PRIVATE;
        else {
          assert (WIN_MASK_COPIED & mbi.Protect);
          page_state = PAGE_COPIED | PAGE_VIRTUAL_PRIVATE;
        }

        check_sharing = true;

      } else
        page_state = PAGE_NOT_COMMITTED;

      const Page* rgn_end = mbi.end ();

      assert (rgn_end <= line_end->pages);

      do {
        *(page_state_ptr++) = page_state;
      } while (++page < rgn_end);

      if (rgn_end >= line_end->pages)
        break;

      mbi.at (rgn_end);
    }
  }

  if (check_sharing) {

    // Check sharing state of pages

    // Work with mapping list - lock it
    Lock lock (m_critical_section);

    for (
      const Line* shared_line = logical_line (line).shared_next ();
      shared_line != line;
      shared_line = logical_line (shared_line).shared_next ()
    ) {

      const Page* page = shared_line->pages;
      Octet* page_state_ptr = state.m_page_states;
      Octet* page_state_end = state.m_page_states + PAGES_PER_LINE;
      check_sharing = false;

      do {
        if (*page_state_ptr & PAGE_VIRTUAL_PRIVATE) {

          MemoryBasicInformation mbi (page);
          const Page* rgn_end = mbi.end ();
          assert (rgn_end <= (shared_line + 1)->pages);

          if (WIN_MASK_MAPPED & mbi.Protect) {
            do {
              *(page_state_ptr++) &= ~PAGE_VIRTUAL_PRIVATE;
            } while (++page < rgn_end);
          } else {
            check_sharing = true;
            page_state_ptr += rgn_end - page;
            page = rgn_end;
          }
        } else
          ++page_state_ptr;

      } while (page_state_ptr < page_state_end);

      if (!check_sharing)
        break;
    }
  }
}