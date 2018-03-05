#include "SyncDomainMemory.h"
#include <algorithm>

using namespace std;

SyncDomainMemory::SyncDomainMemory ()
{
  m_heaps_begin = m_heaps_end = m_empty_heaps_end = m_initial_heaps;
  m_heaps_limit = m_initial_heaps + INITIAL_HEAPS_SPACE;
}

SyncDomainMemory::~SyncDomainMemory ()
{
  // TODO: Need some clearing/diagnostic here
  assert (m_heaps_begin == m_initial_heaps);
  //assert (m_heaps_begin == m_heaps_end);
  
  while (m_empty_heaps_end > m_heaps_end)
    (--m_empty_heaps_end)->destroy ();
}
  
Pointer SyncDomainMemory::allocate (Pointer dst, UWord size, UWord flags)
{  
  if (!size)
    throw BAD_PARAM ();

  // Read-only block are always allocated at protection domain level
  if ((!(flags & READ_ONLY)) && (size <= HEAP_UNIT_MAX)) {
      
    if (dst) {
      
      Heap* ph = lower_bound (m_heaps_begin, m_heaps_end, (Octet*)dst - HEAP_HEADER_SIZE - HEAP_SIZE);
      if ((ph != m_heaps_end) && (ph->header () < dst)) {
        Pointer p = ph->allocate (dst, size, flags);
        if (p || (flags & EXACTLY))
          return p;
      }

    } else {

      if (flags & EXACTLY)
        throw BAD_PARAM ();

      Heap* ph;
#if (HEAP_PARTS > 1)
      Heap* ph_free_part = 0;
#endif
      for (ph = m_heaps_begin; ph < m_heaps_end; ++ph) {
        Pointer p = ph->allocate (0, size, flags);
        if (p)
          return p;
#if (HEAP_PARTS > 1)
        if ((!ph_free_part) && ((~((~0) << HEAP_PARTS)) ^ ph->partitions ()))
          ph_free_part = ph;
#endif
      }

#if (HEAP_PARTS > 1)
      if (ph_free_part)
        return ph_free_part->allocate_in_new_partition (size, flags);
#endif

      // Get new heap
      if (m_heaps_end >= m_empty_heaps_end) {
        assert (m_heaps_end < m_heaps_limit);
        m_heaps_end->create ();
        ++m_empty_heaps_end;
      }

      // Insert in sorted array
      Heap* ins = lower_bound (m_heaps_begin, m_heaps_end, *m_heaps_end);
      if (ins != m_heaps_end) {
        Heap tmp = *m_heaps_end;
        copy (ins + 1, ins, (m_heaps_end - ins) * sizeof (Heap), READ_WRITE);
        *ins = tmp;
      }

      if (++m_heaps_end >= m_heaps_limit) {

        // Reallocate
        UWord cb_cur = (m_heaps_end - m_heaps_begin) * sizeof (Heap);
        if (m_heaps_begin == m_initial_heaps) {
          Pointer pnew = allocate (0, cb_cur * 2, READ_WRITE);
          copy (pnew, m_heaps_begin, cb_cur, READ_WRITE);
        } else
          m_heaps_begin = (Heap*)reallocate (m_heaps_begin, cb_cur, cb_cur * 2, READ_WRITE);

        m_heaps_end = m_empty_heaps_end = m_heaps_begin + cb_cur / sizeof (Heap);
        cb_cur *= 2;
        m_heaps_limit = m_heaps_begin + cb_cur / sizeof (Heap);
      }

      // Now allocate can not fail
      return ins->allocate (0, size, flags);
    }
  }
  
  return large_block_allocate (dst, size, flags);
}

void SyncDomainMemory::release (Pointer p, UWord size)
{
  if (p && size) {

    Octet* end = (Octet*)p + size;
    
    Heap* ph = lower_bound (m_heaps_begin, m_heaps_end, (Octet*)p - HEAP_HEADER_SIZE - HEAP_SIZE);

    if (ph != m_heaps_end) {

      if (p >= ph->header ()) {

        // It is possible that first part of block is in heap, and remaining part 
        // is large block.
        Octet* end_in_heap = ph->space_end ();
        if (end_in_heap > end)
          end_in_heap = end;
      
        // First, check that first part is allocated in heap (can throw).
        ph->check_allocated (p, end);
      
        // Then, try to release large block tail, if there.
        if (end > end_in_heap) {

#ifndef COLLECT_LARGE_BLOCKS
          // Check for next heap is untouchable, else throw.
          Heap* next = ph + 1;
          if (next < m_heaps_end)
            if ((Octet*)next->header () < end)
              throw BAD_PARAM ();
#endif

          // Release large block (can throw).
          large_block_release (end_in_heap, end - end_in_heap);
        }
        
        // Now, release from heap (need not check_allocated inside).
        ph->release (p, size);

        // Check for heap is empty now
        if (ph->empty ()) {
          Heap t = *ph;
          copy (ph, ph + 1, (m_empty_heaps_end - ph - 1) * sizeof (Heap), READ_WRITE);
          *(--m_heaps_end) = t;

          // Purge excess heaps
          if (m_empty_heaps_end - m_heaps_end > 1) {
            (--m_empty_heaps_end)->destroy ();

            if (m_heaps_begin != m_initial_heaps) {
              UWord size = (m_empty_heaps_end - m_heaps_begin) * sizeof (Heap);
              if (size <= INITIAL_HEAPS_SPACE * sizeof (Heap)) {
                copy (m_initial_heaps, m_heaps_begin, size, READ_WRITE);
                m_heaps_end = m_initial_heaps + (m_heaps_end - m_heaps_begin);
                m_empty_heaps_end = m_initial_heaps + (m_empty_heaps_end - m_heaps_begin);
                size = (m_heaps_limit - m_heaps_begin) * sizeof (Heap);
                m_heaps_limit = m_initial_heaps + INITIAL_HEAPS_SPACE;
                void* p = m_heaps_begin;
                m_heaps_begin = m_initial_heaps;
                release (p, size);
              }
            }
          }
        }

        return;
      }

#ifndef COLLECT_LARGE_BLOCKS
      else {
        // Check for next heap is untouchable, else throw.
        if (ph < m_heaps_end)
          if ((Octet*)ph->header () < end)
            throw BAD_PARAM ();
      }
#endif
        
    }
    
    large_block_release (p, size);
  }
}

Word SyncDomainMemory::query (Pointer p, QueryParam q)
{
  Heap* ph = lower_bound (m_heaps_begin, m_heaps_end, (Octet*)p - HEAP_HEADER_SIZE - HEAP_SIZE);
  if (ph != m_heaps_end && p > ph->space ()) {

    // TODO: More information here (region begin, region end).
    switch (q) {

    case ALLOCATION_UNIT:
      return HEAP_UNIT_MIN;

    }
  }

  return system_memory ()->query (p, q);
}

Pointer SyncDomainMemory::copy (Pointer dst, Pointer src, UWord size, UWord flags)
{
  // TODO: Need some optimization here.
  return system_memory ()->copy (dst, src, size, flags);
}

void SyncDomainMemory::commit (Pointer dst, UWord size)
{
  system_memory ()->commit (dst, size);
}

void SyncDomainMemory::decommit (Pointer dst, UWord size)
{
  system_memory ()->decommit (dst, size);
}