#include "MemInfo.h"
#include "CostOfOperation.h"
#include "LineRegions.h"

Pointer WinMemory::copy (Pointer dst, Pointer src, UWord size, UWord flags)
{
  if (!src)
    throw BAD_PARAM ();
  
  if (!size)
    return 0;

  // Check source data
  if (IsBadReadPtr (src, size))
    throw BAD_PARAM ();

  switch (flags & (DECOMMIT | RELEASE)) {

  case 0:
    break;

  case DECOMMIT:
    if (check_allocated (Page::begin (src), Page::end ((Octet*)src + size)) & ~WIN_MASK_WRITE)
      throw BAD_PARAM ();
    break;

  case RELEASE:
    check_allocated (Page::begin (src), Page::end ((Octet*)src + size));
    break;

  default:
    throw INV_FLAG ();
  }

  // Destination must be new allocated?
  if ((!dst) || (flags & ALLOCATE)) {
  
    // Allocate new block
    flags |= ALLOCATE;
    flags &= ~ZERO_INIT;

    // To use sharing, source and destination must have same offset from line begin
    // TODO: Not in all cases it will be optimal (small pieces across page boundaries). 
    
    UWord line_offset = (UWord)src & (LINE_SIZE - 1);

    if (dst) {  // Allocation address explicitly specified

      if (((UWord)dst & (LINE_SIZE - 1)) == line_offset) {

        UWord page_offset = line_offset & (PAGE_SIZE - 1);

        if (!reserve (size + page_offset, flags, Line::begin (dst))) {

          if (flags & EXACTLY)
            return 0;

          dst = 0;
        }

      } else
          dst = 0;
    }

    if (!dst) {

      if (flags & EXACTLY)
        throw BAD_PARAM ();

      // Allocate block with same alignment as source
      Line* dst_begin_line = reserve (size + line_offset, flags);
      dst = dst_begin_line->pages->bytes + line_offset;
    }

  } else {

    // Validate destination block

    UWord dst_protect_mask;
    if (flags & READ_ONLY)
      dst_protect_mask = ~WIN_MASK_READ;
    else
      dst_protect_mask = ~WIN_MASK_WRITE;
    
    if (check_allocated (Page::begin (dst), Page::end ((Octet*)dst + size)) & dst_protect_mask)
      throw BAD_PARAM (); // some destination lines allocated with different protection
  }

  // Do copy
  try {

    Octet* dst_end = (Octet*)dst + size;

    if (dst == src)
      if (flags & ALLOCATE)
        throw BAD_PARAM ();
      else
        return dst;

    // Always perform copy line-by-line, aligned to destination lines

    if (dst < src) {

      void* dst_begin = dst;
      do {

        Octet* dst_line_end = Line::end (dst_begin)->pages->bytes;
        if (dst_line_end > dst_end)
          dst_line_end = dst_end;

        UWord size = dst_line_end - (Octet*)dst;
        copy_one_line ((Octet*)dst_begin, (Octet*)src, size, flags);

        src = (Octet*)src + size;
        dst_begin = dst_line_end;

      } while (dst_begin < dst_end);

    } else {

      do {

        Octet* dst_line_begin = Line::begin (dst_end - 1)->pages->bytes;
        
        if (dst_line_begin < (Octet*)dst)
          dst_line_begin = (Octet*)dst;
        Octet* src_line_begin = (Octet*)src + (dst_line_begin - (Octet*)dst);

        UWord size = dst_end - dst_line_begin;
        copy_one_line (dst_line_begin, src_line_begin, size, flags);
      
        dst_end = dst_line_begin;

      } while (dst_end > dst);

    }

  } catch (...) {
    
    if (flags & ALLOCATE)
      release (Line::begin (dst), Line::end ((Octet*)dst + size));
    
    throw;
  }

  if (RELEASE == (flags & RELEASE)) {
    
    // Release memory. DECOMMIT implemented inside copy_one_line.
    
    Line* release_begin = Line::begin (src);
    Line* release_end = Line::end ((Octet*)src + size);

    if (dst < src) {
      Line* dst_end = Line::end ((Octet*)dst + size);
      if (release_begin < dst_end)
        release_begin = dst_end;
    } else {
      Line* dst_begin = Line::begin (dst);
      if (release_end > dst_begin)
        release_end = dst_begin;
    }

    if (release_begin < release_end)
      release (release_begin, release_end);
  }
  
  return dst;
}

void WinMemory::copy_one_line (Octet* dst_begin, Octet* src_begin, UWord size, UWord flags)
{
  assert (src_begin);
  assert (dst_begin);
  assert (dst_begin != src_begin);
  assert (size);
  assert ((dst_begin + size) <= Line::end (dst_begin + 1)->pages->bytes);

  // To use sharing, source and destination must have same offset from line begin.

  if (!(
    (((UWord)dst_begin & (LINE_SIZE - 1)) == ((UWord)src_begin & (LINE_SIZE - 1)))
  && 
    copy_one_line_aligned (dst_begin, src_begin, size, flags)
  )) { 
    
    // Perform simple copy

    LineState dst_line_state;
    get_line_state (Line::begin (dst_begin), dst_line_state);
    
    CostOfOperation cost;
    commit_line_cost (dst_begin, dst_begin + size, dst_line_state, cost, true);
    
    copy_one_line_really (dst_begin, src_begin, size, cost.decide_remap (), dst_line_state);
  }

  // Освобождаем исходные страницы, если нужно.
  // Вместо release делаем decommit. Иначе другой тред может успеть занять область внутри копируемого участка.
  if (flags &= (DECOMMIT | RELEASE)) {

    Page* begin, * end;
    if (RELEASE == flags) {
      begin = Line::begin (src_begin)->pages;
      end = Line::end (src_begin + size)->pages;
    } else {
      begin = Page::end (src_begin);
      end = Page::begin (src_begin + size);
    }

    if (dst_begin < src_begin) {
      Page* dst_end = Page::end (dst_begin + size);
      if (begin < dst_end)
        begin = dst_end;
    } else {
      Page* pg_begin = Page::begin (dst_begin);
      if (end > pg_begin)
        end = pg_begin;
    }

    if (begin < end)
      decommit (begin, end);
  }
}

bool WinMemory::copy_one_line_aligned (Octet* dst_begin, Octet* src_begin, UWord size, UWord flags)
{
  // We must minimize two parameters: amount of copied data
  // and amount of allocated physical memory.

  // Будем считать, что копирование каждого байта добавляет к стоимости операции единицу.
  // Выделение физической страницы добавляет PAGE_ALLOCATE_COST единиц.

  // We can copy by two ways:
  // 1. Sharing
  // 1.1. Re-map source line (data copying possible)
  // 1.2. Map destination line at temporary address
  // 1.3. Map source line to destination
  // 1.4. Copy unchanged data back to destination.
  // 1.5. Unmap temporary mapping
  // 2. Copying
  // 2.1. Commit destination pages
  // 2.2. Copy source data to destination

  // We must choose optimal way now

  /*
  При подсчете объема данных, копируемых при выполнении операции, необходимо учитывать
  не только те данные, которые копируем мы, но и те, которые копирует система, 
  если страница copy-on-write и мы пишем в нее.
  */

  HANDLE src_mapping = mapping (src_begin);
  if (src_mapping) {
  
    // Source line allocated by this service

    Octet* dst_end = dst_begin + size;

    // Определяем диапазон целевых страниц, которым соответствуют исходные страницы,
    // освобождающиеся в процессе копирования.
    // Для простоты вычисления воспользуемся тем, что исходная и целевая области
    // имеют одинаковое смещение относительно начала строки.
    Page* dst_release_begin;
    Page* dst_release_end;

    switch (flags & (DECOMMIT | RELEASE)) {

    case 0:
      dst_release_begin = 0;
      dst_release_end = 0;
      break;

    case DECOMMIT:
      dst_release_begin = Page::end (dst_begin);
      dst_release_end = Page::begin (dst_end);
      break;

    case RELEASE:
      dst_release_begin = Page::begin (dst_begin);
      dst_release_end = Page::end (dst_end);
      break;

    default:
      assert (false);
    }

    HANDLE dst_mapping = mapping (dst_begin); // can be == src_mapping

    // Получаем состояния страниц для исходной и целевой строк.

    Line* src_line = Line::begin (src_begin);
    LineState src_line_state;
    get_line_state (src_line, src_line_state);
    assert (src_line_state.m_allocation_protect); // not free

    Line* dst_line = Line::begin (dst_begin);
    LineState dst_line_state;
    get_line_state (dst_line, dst_line_state);
    assert (dst_line_state.m_allocation_protect); // not free

    // Анализируем состояния страниц, определяем возможность и стоимость отображения.

    CostOfOperation cost_of_share;  // стоимость разделения

    bool need_full_remap = false;
    // если true, разделение возможно только после полного переотображения

    LineRegions bytes_in_place, bytes_not_mapped; // см. ниже
    
    Octet* src_page_state_ptr = src_line_state.m_page_states;
    Octet* dst_page_state_ptr = dst_line_state.m_page_states;
    Octet* dst_page_state_end = dst_line_state.m_page_states + PAGES_PER_LINE;
    Page* dst_page = dst_line->pages;

    do {

      Octet src_page_state = *src_page_state_ptr;
      Octet dst_page_state = *dst_page_state_ptr;

      if (dst_page_state & PAGE_COMMITTED) {

        // Если исходная и целевая логические страницы не отображены на общую виртуальную,
        if ((src_mapping != dst_mapping) || (src_page_state != PAGE_MAPPED_SHARED) || (dst_page_state != PAGE_MAPPED_SHARED)) {
        
          // Если целевая страница не разделена, она будет освобождена
          if (dst_page_state & PAGE_VIRTUAL_PRIVATE)
            cost_of_share += -PAGE_ALLOCATE_COST;

          // Определяем количество байт, которые должны остаться на целевой странице.
          Word dst_bytes = dst_begin - dst_page->bytes;
          if (dst_bytes >= PAGE_SIZE) {

            // Страница целиком перед областью копирования
            dst_bytes = PAGE_SIZE;

            // При разделении, эта страница должна быть скопирована в исходный маппинг.
            bytes_in_place.push_back (dst_page->bytes - dst_line->pages->bytes, PAGE_SIZE);

          } else {

            if (dst_bytes <= 0)
              dst_bytes = 0;
            else
              // При разделении, dst_bytes из начала страницы должны быть скопированы
              // в исходный маппинг.
              bytes_in_place.push_back (dst_page->bytes - dst_line->pages->bytes, dst_bytes);

            Word bytes = dst_page->bytes + PAGE_SIZE - dst_end;
            if (bytes >= PAGE_SIZE) {

              // Страница целиком за областью копирования.
              bytes = PAGE_SIZE;

              // При разделении, эта страница должна быть скопирована в исходный маппинг.
              bytes_in_place.push_back (dst_page->bytes - dst_line->pages->bytes, PAGE_SIZE);

            } else if (bytes <= 0)
              bytes = 0;
            else
              // При разделении, bytes из конца страницы должны быть скопированы
              // в исходный маппинг.
              bytes_in_place.push_back (dst_end - dst_line->pages->bytes, bytes);
          
            dst_bytes += bytes;
          }

          if (dst_bytes) {

            // На целевой странице должны остаться dst_bytes.
            // При отображении они должны попасть в исходный маппинг.
            // Чтобы не усложнять алгоритм отображения, примем требование,
            // что исходные виртуальная и логическая страницы должны быть свободны
            // или освобождаться. При этом целевую страницу можно отобразить приватно.
            // В противном случае, отображение будем считать невозможным.

            if (
              // исходная страница уже содержит данные,
              (src_page_state & PAGE_COMMITTED)
            && // и мы не собираемся ее освобождать
              ((dst_page >= dst_release_end) || (dst_page < dst_release_begin))
            ) {

              // считаем отображение невозможным.
              break;
            }

            if (!(src_page_state & PAGE_VIRTUAL_PRIVATE))
              // Исходная виртуальная страница занята.
              // Разделение возможно только после полного переотображения,
              // когда она освободится.
              need_full_remap = true;

            // Учитываем стоимость копирования dst_bytes в исходный маппинг.
            cost_of_share += dst_bytes;
          }
        }
      }

      // Проверяем необходимость переотображения исходной страницы
      if ((src_page_state & PAGE_COPIED) == PAGE_COPIED) {

        // Исходная страница нуждается в переотображении.
        // Подсчитаем количество байт, попадающих в область копирования.
        Page* page_end = dst_page + 1;
        Octet* copy_begin = max (dst_begin, dst_page->bytes);
        Word src_bytes = min (dst_end, page_end->bytes) - copy_begin;
        if (src_bytes > 0) {
        
          // Если не делать переотображение, надо скопировать src_bytes,
          // при этом будет выделена новая страница.
          cost_of_share [REMAP_NONE] += src_bytes + PAGE_ALLOCATE_COST;
        
          // Запомним не отображенный блок.
          bytes_not_mapped.push_back (copy_begin - dst_line->pages->bytes, src_bytes);

          // Если виртуальная страница разделена, она будет скопирована системой при записи.
          if (!(src_page_state & PAGE_VIRTUAL_PRIVATE))
            cost_of_share [REMAP_NONE] += PAGE_SIZE;

          // Если исходная страница освобождается, вычтем ее из стоимости.
          if ((dst_page < dst_release_end) && (dst_page >= dst_release_begin))
            cost_of_share [REMAP_NONE] -= PAGE_ALLOCATE_COST;
        }

        // Тип переотображения зависит от занятости виртуальной страницы.
        cost_of_share.remap_type ((src_page_state & PAGE_VIRTUAL_PRIVATE) ? REMAP_PART : REMAP_FULL);
      }

      ++dst_page;
      ++src_page_state_ptr;
      ++dst_page_state_ptr;
    } while (dst_page_state_ptr < dst_page_state_end);

    if (dst_page_state_ptr >= dst_page_state_end) {

      // Разделение возможно.
      bool can_share = true;

      // Decide source line remap type for sharing
      RemapType remap_share;
      
      // No remapping possible for 'this' object location.        
      if (((void*)this >= src_line) && ((void*)this < (src_line + 1))) {
        
        if (need_full_remap)
          can_share = false;
        else
          remap_share = REMAP_NONE;

      } else
        remap_share = need_full_remap ? REMAP_FULL : cost_of_share.decide_remap ();

      // Calculate cost of simple copying.
      
      CostOfOperation cost_of_copy = size;
      commit_line_cost (dst_begin, dst_end, dst_line_state, cost_of_copy, true);
      
      // Decide destination line remap type for simple copy.
      RemapType remap_copy;

      // No remapping possible for 'this' object location.        
      if (((void*)this >= dst_line) && ((void*)this < (dst_line + 1)))
        remap_copy = REMAP_NONE;
      else
        remap_copy = cost_of_copy.decide_remap ();

      // Выбираем оптимальный вариант.
      if (can_share && (cost_of_share [remap_share] <= cost_of_copy [remap_copy])) {

        // Копируем путем разделения.

        // Переотображаем исходную строку.
        if (remap_share != REMAP_NONE)
          src_mapping = remap_line (src_line->pages->bytes, src_line->pages->bytes, src_line_state, remap_share);

        // Копируем даные целевой строки, которые должны остаться на месте.
        if (bytes_in_place.not_empty ()) {

          Line* tmp_line;
          // Виртуальные страницы, в которые мы копируем, заведомо свободны, это мы проверили раньше.
          // Однако, если переотображение не выполнялось, они могут быть не отображены на исходную строку.
          // Поэтому, если remap_share == REMAP_NONE, отображаем исходную виртуальную строку
          // по временному адресу.

          if (remap_share == REMAP_NONE) {
            if (!(tmp_line = (Line*)MapViewOfFileEx (src_mapping, FILE_MAP_WRITE, 0, 0, LINE_SIZE, 0)))
              throw NO_MEMORY ();
          } else
            tmp_line = src_line;

          for (LineRegions::Iterator it = bytes_in_place.begin (); it != bytes_in_place.end (); ++it) {

            Octet* dst = tmp_line->pages->bytes + it->offset;

            // Даже если страница read-only, VirtualAlloc сделает ее read-write
            if (!VirtualAlloc (dst, it->size, MEM_COMMIT, PAGE_READWRITE))
              throw NO_MEMORY ();

            Octet* src_begin = dst_line->pages->bytes + it->offset;
            Octet* src_end = src_begin + it->size;
            real_copy (src_begin, src_end, dst);

            // Запрещаем доступ из исходной строки.
            UWord old;
            verify (VirtualProtect (src_line->pages->bytes + it->offset, it->size, WIN_NO_ACCESS, &old));
          }

          // Освобождаем временное отображение.
          if (tmp_line != src_line)
            UnmapViewOfFile (tmp_line);
        }

        // Отображаем исходную строку на целевую.
        map (dst_line, src_line);

        // Вычисляем новые состояния страниц целевой строки.

        src_page_state_ptr = src_line_state.m_page_states;
        dst_page_state_ptr = dst_line_state.m_page_states;
        dst_page = dst_line->pages;

        UWord prot_private, prot_shared;
        if (WIN_MASK_WRITE & dst_line_state.m_allocation_protect) {
          prot_private = WIN_WRITE_MAPPED_PRIVATE;
          prot_shared = WIN_WRITE_MAPPED_SHARED;
        } else {
          prot_private =
          prot_shared = WIN_READ_MAPPED;
        }

        do {

          UWord old;

          Page* next_page = dst_page + 1;

          // Если целевая страница содержит данные, которые нужно сохранить,
          // она всегда отображается приватно.
          if (
            (*dst_page_state_ptr & PAGE_COMMITTED)
          &&
            ((dst_page->bytes < dst_begin) || (next_page->bytes > dst_end))
          ) {
            
            // Целевая страница содержит данные и находится вне области копирования, по крайней мере, частично.
            // Сохраняемые целевые страницы всегда отображаются приватно.
            // Исходная страница не должна быть в маппинге или должна освобождаться.
            assert ((PAGE_NOT_COMMITTED == *src_page_state_ptr) || (PAGE_COPIED == *src_page_state_ptr) || ((dst_page >= dst_release_begin) && (dst_page < dst_release_end)));
            verify (VirtualProtect (dst_page, PAGE_SIZE, prot_private, &old));
            *dst_page_state_ptr = PAGE_MAPPED_PRIVATE;

          } else if ((dst_page->bytes < dst_end) && (next_page->bytes > dst_begin)) {

            // Страница в области копирования

            if ((dst_page < dst_release_end) && (dst_page >= dst_release_begin)) {

              // Страница соответствует освобождающейся исходной странице.
              // Приватность сохраняется.
              if (*src_page_state_ptr & PAGE_VIRTUAL_PRIVATE) {
                verify (VirtualProtect (dst_page, PAGE_SIZE, prot_private, &old));
                *dst_page_state_ptr = PAGE_MAPPED_PRIVATE;
              } else {
                verify (VirtualProtect (dst_page, PAGE_SIZE, prot_shared, &old));
                *dst_page_state_ptr = PAGE_MAPPED_SHARED;
              }

            } else {

              // Страница разделяется.
              verify (VirtualProtect (dst_page, PAGE_SIZE, prot_shared, &old));
              *dst_page_state_ptr = PAGE_MAPPED_SHARED;
            }
          }

          ++dst_page;
          ++src_page_state_ptr;
          ++dst_page_state_ptr;
        } while (dst_page_state_ptr < dst_page_state_end);

        // Если переотображение не выполнялось, физически копируем не отображенные данные.
        if (remap_share == REMAP_NONE)
          for (LineRegions::Iterator it = bytes_not_mapped.begin (); it != bytes_not_mapped.end (); ++it)
            copy_one_line_really (dst_line->pages->bytes + it->offset, src_line->pages->bytes + it->offset, it->size, REMAP_NONE, dst_line_state);

        // Если исходная строка read-write,
        if (WIN_MASK_WRITE & src_line_state.m_allocation_protect) {
          // надо поменять защиту разделенных страниц на copy-on-write.
          // Если страница не отображена, она останется read-write.
          UWord old;
          verify (VirtualProtect (src_begin, size, WIN_WRITE_MAPPED_SHARED, &old));
        }

      } else
        // Выполняем простое копирование, состояние целевой страницы проанализировано.
        copy_one_line_really (dst_begin, src_begin, size, remap_copy, dst_line_state);

      return true; // Копирование выполнено.
      
    }

  } else
    // Source line is not allocated by this service
    assert (!(flags & (DECOMMIT | RELEASE)));

  return false;
}
  
void WinMemory::copy_one_line_really (Octet* dst_begin, const Octet* src_begin, UWord size, RemapType remap_type, LineState& dst_line_state)
{
  assert (size);

  Octet* dst_end = dst_begin + size;
  
  if (remap_type != REMAP_NONE)
    remap_line (dst_begin, dst_end, dst_line_state, remap_type);
  
  // Commit pages
  Page* dst_begin_page = Page::begin (dst_begin);
  Page* dst_end_page = Page::end (dst_end);
  commit_one_line (dst_begin_page, dst_end_page, dst_line_state, false);
  
  // Do copy
  real_move (src_begin, src_begin + size, dst_begin);

  if (WIN_MASK_READ & dst_line_state.m_allocation_protect) {
    // Restore read-only protection

    Octet* page_state_ptr = dst_line_state.m_page_states + (dst_begin_page - Line::begin (dst_begin)->pages);
    Page* page = dst_begin_page;
    do {

      UWord old;
      verify (VirtualProtect (page, PAGE_SIZE, 
        ((*page_state_ptr & PAGE_COPIED) == PAGE_COPIED) ? WIN_READ_COPIED : WIN_READ_MAPPED,
        &old));
        
      ++page_state_ptr;
      ++page;
    } while (page < dst_end_page);
  }
}
