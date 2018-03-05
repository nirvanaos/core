// Nirvana project
// SyncDomainMemory - synchronization domain memory service.

#ifndef _SYNC_DOMAIN_MEMORY_H_
#define _SYNC_DOMAIN_MEMORY_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#define COLLECT_LARGE_BLOCKS

#include "Heap.h"
#ifdef COLLECT_LARGE_BLOCKS
#include <set>
using namespace std;
#endif

using namespace Nirvana;

// class SyncDomainMemory is base for SyncDomain
class SyncDomainMemory : public Memory
{
public:

  // Memory::
  virtual Pointer allocate (Pointer dst, UWord size, UWord flags);
  virtual Pointer copy (Pointer dst, Pointer src, UWord size, UWord flags);
  virtual void release (Pointer dst, UWord size);
  virtual void commit (Pointer dst, UWord size);
  virtual void decommit (Pointer dst, UWord size);
  virtual Word query (Pointer p, QueryParam q);

  SyncDomainMemory ();
  ~SyncDomainMemory ();

  void init_heap (Heap h)
  {
    assert (m_heaps_begin == m_initial_heaps);
    assert (m_heaps_end == m_heaps_begin);
    m_heaps_end = m_empty_heaps_end = m_heaps_begin + 1;
    *m_heaps_begin = h;
  }

private:

  void large_block_release (void* p, UWord cb)
  {
#ifdef COLLECT_LARGE_BLOCKS
    m_large_blocks.release (p, cb);
#else
    system_memory ()->release (p, cb);
#endif
  }

  void* large_block_allocate (void* p, UWord cb, UWord flags)
  {
#ifdef COLLECT_LARGE_BLOCKS
    return m_large_blocks.allocate (p, cb, flags);
#else
    return system_memory ()->allocate (p, cb, flags);
#endif
  }
  
  enum {
    INITIAL_HEAPS_SPACE = 4
  };

  Heap* m_heaps_begin;
  Heap* m_heaps_end;
  Heap* m_empty_heaps_end;
  Heap* m_heaps_limit;
  Heap  m_initial_heaps [INITIAL_HEAPS_SPACE];

#ifdef COLLECT_LARGE_BLOCKS
  
  struct LargeBlock
  {
    LargeBlock (void* p, UWord size);

    bool operator < (const LargeBlock& rhs) const
    {
      return m_begin < rhs.m_begin;
    }

    Octet* m_begin;
    Octet* m_end;
  };

  class LargeBlocks : public set <LargeBlock> 
  {
  public:
    void allocate (void* dst, UWord size, UWord state);
    bool release (void* p, UWord size);
  };

  LargeBlocks m_large_blocks;

#endif
};

#endif
