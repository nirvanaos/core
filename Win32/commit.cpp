#include "MemInfo.h"
#include "CostOfOperation.h"

void WinMemory::commit (Pointer dst, UWord size)
{
  // Check parameters

  if (!size)
    return;

  if (!dst)
    throw BAD_PARAM ();

  // Align to pages

  Page* begin = Page::begin (dst);
  Page* end = Page::end ((Octet*)dst + size);

  // Check memory state

  if (check_allocated (begin, end) & ~WIN_MASK_WRITE)
    throw BAD_PARAM (); // Some lines not read-write

  // Commit

  commit (begin, end, ZERO_INIT);
}

void WinMemory::commit (Page* begin, Page* end, UWord zero_init)
{
  assert (begin);
  assert (end);
  assert (begin < end);
  assert (!((UWord)begin % PAGE_SIZE));
  assert (!((UWord)end % PAGE_SIZE));

  do {

    // Define line margins

    Page* line_end = Line::end (begin->bytes + 1)->pages;
    if (line_end > end)
      line_end = end;

    Line* line = Line::begin (begin);

    // Get current line state

    LineState line_state;
    get_line_state (line, line_state);

    // Do not remap line if one contains this object
    if (((void*)this < line) || ((void*)this >= (line + 1))) {

      // Calculate operation cost for remapping
    
      CostOfOperation cost;
      commit_line_cost (begin->bytes, end->bytes, line_state, cost, false);

      // Decide remap type

      RemapType remap_type = cost.decide_remap ();
      if (remap_type != REMAP_NONE)
        remap_line (begin->bytes, begin->bytes, line_state, remap_type);
    }

    commit_one_line (begin, line_end, line_state, zero_init);

    begin = line_end;

  } while (begin < end);
}

void WinMemory::commit_line_cost (const Octet* begin, const Octet* end, const LineState& line_state, CostOfOperation& cost, bool copy)
{
  const Page* page = Line::begin (begin)->pages;
  const Page* line_end = page + PAGES_PER_LINE;
  const Octet* page_state_ptr = line_state.m_page_states;

  do {
  
    Octet page_state = *page_state_ptr;
  
    const Page* page_end;

    if ((page->bytes < end) && ((page_end = page + 1)->bytes > begin)) {

      // Страница в области фиксации.
      // Стоимость фиксации должна включать перевод этой страницы в private
      // состояние, поскольку, после commit обычно следует запись в данный диапазон.
      // В частности, это происходит при физическом копировании, где тоже
      // вызывается данная функция.

      // Определим количество байт, которые должны остаться на странице без изменений.

      UWord dst_bytes;
      if (copy) {

        // Подсчитывается стоимость копирования.
        // Байты, находящиеся между begin и end, не участвуют в переотображении.
        Word bytes = begin - page->bytes;
        if (bytes > 0)
          dst_bytes = bytes;
        else
          dst_bytes = 0;

        bytes = page_end->bytes - end;
        if (bytes > 0)
          dst_bytes += bytes;
      } else
        dst_bytes = PAGE_SIZE;

      switch (page_state) {

      case PAGE_MAPPED_SHARED:
    
        // Целевая страница разделена, она будет скопирована системой
        // перед копированием в нее src_bytes.
        // Таким образом, кроме копирования данных,
        // будет выделена и скопирована целая страница.
      
        cost [REMAP_NONE] += PAGE_SIZE + PAGE_ALLOCATE_COST;
        cost [REMAP_PART] += PAGE_SIZE + PAGE_ALLOCATE_COST;

        // При полном переотображении, мы выделим новую страницу и скопируем в нее dst_bytes.

        cost [REMAP_FULL] += dst_bytes + PAGE_ALLOCATE_COST;

        break;
      
      case PAGE_DECOMMITTED | PAGE_VIRTUAL_PRIVATE:
      
        // Страница расфиксирована, виртуальная страница свободна.
        // Страница может быть не отображена. Есть смысл сделать переотображение.
        cost.remap_type (REMAP_PART);
      
      case PAGE_NOT_COMMITTED:
      
        // Виртуальная страница свободна
        // В любом случае, мы просто выделяем новую страницу.
        cost += PAGE_ALLOCATE_COST;
      
        break;
      
      case PAGE_DECOMMITTED:
      
        // Виртуальная страница занята
      
        // Без полного переотображения, мы должны отобразить ее, как copy-on-write.
        // При записи в нее, она будет скопирована системой во вновь выделенную
        // физическую страницу.
      
        cost [REMAP_NONE] += PAGE_SIZE + PAGE_ALLOCATE_COST;
        cost [REMAP_PART] += PAGE_SIZE + PAGE_ALLOCATE_COST;
      
        // При полном переотображении, мы только выделим новую страницу.
      
        cost [REMAP_FULL] += PAGE_ALLOCATE_COST;
      
        break;
      
      case PAGE_MAPPED_PRIVATE:
      
        // Страница отображена и не разделена.
        // При полном переотображении надо скопировать dst_bytes.
      
        cost [REMAP_FULL] += dst_bytes;
      
        break;
      
      case PAGE_COPIED:
      
        // Страница не отображена, виртуальная страница занята.
        // Если будет переотображение, оно должно быть полным.
        cost.remap_type (REMAP_FULL);
      
      case PAGE_COPIED | PAGE_VIRTUAL_PRIVATE:
      
        // Страница не отображена.
        cost.remap_type (REMAP_PART);
      
        // При любом переотображении, страницу надо скопировать.
        cost [REMAP_PART] += dst_bytes;
        cost [REMAP_FULL] += dst_bytes;
      
        break;
      
      default:
        assert (false);
      }

    } else {

      // Страница вне области копирования
      if (PAGE_COMMITTED & page_state) {

        // При полном переотображении копируем любую зафиксированную страницу.
      
        cost [REMAP_FULL] += PAGE_SIZE;
      
        if (PAGE_MAPPED_SHARED == page_state)
        
          // Страница разделена, при переотображении будет выделена новая.
          cost [REMAP_FULL] += PAGE_ALLOCATE_COST;
      
        else if (page_state & PAGE_COPIED == PAGE_COPIED) {
        
          // Страница не отображена, переотображение имеет смысл.
        
          // При частичном переотображении она должна быть скопирована.
          cost [REMAP_PART] += PAGE_SIZE;
        
          // Тип переотображения зависит от занятости виртуальной страницы.
          cost.remap_type ((PAGE_VIRTUAL_PRIVATE & page_state) ? REMAP_PART : REMAP_FULL);
        }
      }
    }

    ++page_state_ptr;
    ++page;
  } while (page < line_end);
}

void WinMemory::commit_one_line (Page* begin, Page* end, LineState& line_state, UWord zero_init)
{
  assert (line_state.m_allocation_protect); // not free

  Line* begin_line = Line::begin (begin);

  assert (Line::end (end) == begin_line + 1);

  map (begin_line);
  
  Octet* page_state_ptr = line_state.m_page_states + (UWord)begin / PAGE_SIZE % PAGES_PER_LINE;
  Octet* page_state_end = line_state.m_page_states + (UWord)end / PAGE_SIZE % PAGES_PER_LINE;
  
  do {
    
    if (PAGE_NOT_COMMITTED == *page_state_ptr) {

      // These pages are not committed anywhere
        
      // Calculate region size
      UWord size = PAGE_SIZE;
      for (;;) {
        if (++page_state_ptr >= page_state_end)
          break;
        if (PAGE_NOT_COMMITTED != *page_state_ptr)
          break;
        size += PAGE_SIZE;
      }
        
      // Commit in this line
      if (!VirtualAlloc (begin, size, MEM_COMMIT, WIN_WRITE_MAPPED_PRIVATE))
        throw NO_MEMORY ();
        
      // Disable access at other shared lines
      UWord offset = (begin - begin_line->pages) * PAGE_SIZE;
      for (
        Line* shared_line = logical_line (begin_line).shared_next ();
        shared_line != begin_line;
        shared_line = logical_line (shared_line).shared_next ()
      ) 
        decommit_surrogate ((Page*)(shared_line->pages->bytes + offset), size);
        
      *page_state_ptr = PAGE_MAPPED_PRIVATE;
      
      begin = (Page*)(begin->bytes + size);

    } else {
      
      if (PAGE_DECOMMITTED & *page_state_ptr) {

        // Recommit
        UWord old_protect;
        verify (VirtualProtect (begin, PAGE_SIZE, WIN_WRITE_MAPPED_SHARED, &old_protect));
        
        // Если страница отображена, система присвоит ей защиту
        // WIN_WRITE_MAPPED_SHARED, иначе - WIN_WRITE_COPIED.
      
        // Если страница отображена на неразделенную виртуальную страницу,
        // нужно изменить защиту WIN_WRITE_MAPPED_SHARED на WIN_WRITE_MAPPED_PRIVATE.
        
        if (*page_state_ptr & PAGE_VIRTUAL_PRIVATE) {

          // Виртуальная страница не разделена
        
          MemoryBasicInformation mbi (begin);
        
          if (WIN_WRITE_MAPPED_SHARED == mbi.Protect) {
            UWord old;
            verify (VirtualProtect (begin, PAGE_SIZE, WIN_WRITE_MAPPED_PRIVATE, &old));
            *page_state_ptr = PAGE_MAPPED_PRIVATE;
          } else
            *page_state_ptr = PAGE_COPIED | PAGE_VIRTUAL_PRIVATE;

        } else
          *page_state_ptr = PAGE_COPIED;
      
        if (zero_init && (WIN_NO_ACCESS == old_protect)) {

          // Zero-init recommitted page

          UWord* pw = (UWord*)begin;
          UWord* page_end = (UWord*)(begin + 1);
        
          do {
            if (*pw) {
              do {
                *pw = 0;
              } while (++pw < page_end);
              break;
            }
          } while (++pw < page_end);
        }
      }

      ++begin;
      ++page_state_ptr;
    }
  } while (begin < end);
}