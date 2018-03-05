// Nirvana System ORB core implementation definitions

#ifndef _CORE_H_
#define _CORE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <nirvana.h>
#include <assert.h>
#include "config.h"

#undef verify

#ifdef NDEBUG

#define verify(exp) (exp)

#else

#define verify(exp) assert(exp)

#endif

using namespace Nirvana;
using namespace CORBA;

namespace Nirvana {

template <class InIt, class OutIt>
inline OutIt real_copy (InIt begin, InIt end, OutIt dst)
{
  while (begin != end)
    *(dst++) = *(begin++);

  return dst;
}

// Partial specialization for performance

template <>
inline Octet* real_copy (const Octet* begin, const Octet* end, Octet* dst)
{
  Octet* aligned_begin = (Octet*)(((UWord)begin + sizeof (UWord) - 1) & ~(sizeof (UWord) - 1));
  Octet* aligned_end = (Octet*)(((UWord)end + sizeof (UWord) - 1) & ~(sizeof (UWord) - 1));
  
  if (aligned_begin < aligned_end) {
    
    while (begin != aligned_begin)
      *(dst++) = *(begin++);
    
    do {
      *(UWord*)dst = *(UWord*)begin;
      dst += sizeof (UWord);
      begin += sizeof (UWord);
    } while (begin != aligned_end);
    
  }

  while (begin != end)
    *(dst++) = *(begin++);
  
  return dst;
}

template <class BidIt1, class BidIt2>
inline void real_move (BidIt1 begin, BidIt1 end, BidIt2 dst)
{
  if (dst <= begin || dst >= end)
    while (begin != end)
      *(dst++) = *(begin++);
  else
    while (begin != end)
      *(--dst) = *(--end);
}

// Partial specialization for performance

template <>
inline void real_move (const Octet* begin, const Octet* end, Octet* dst)
{
  Octet* aligned_begin = (Octet*)(((UWord)begin + sizeof (UWord) - 1) & ~(sizeof (UWord) - 1));
  Octet* aligned_end = (Octet*)(((UWord)end + sizeof (UWord) - 1) & ~(sizeof (UWord) - 1));
  
  if (dst <= begin || dst >= end) {

    if (aligned_begin != aligned_end) {
      
      while (begin != aligned_begin)
        *(dst++) = *(begin++);
      
      do {
        *(UWord*)dst = *(UWord*)begin;
        dst += sizeof (UWord);
        begin += sizeof (UWord);
      } while (begin != aligned_end);
      
    }
    
    while (begin != end)
      *(dst++) = *(begin++);
    
  } else {
    
    if (aligned_begin < aligned_end) {
      
      while (end != aligned_end)
        *(--dst) = *(--end);
      
      do {
        dst -= sizeof (UWord);
        end -= sizeof (UWord);
        *(UWord*)dst = *(UWord*)end;
      } while (end != aligned_begin);
      
    }
    
    while (end != begin)
      *(--dst) = *(--end);
    
  }
}

inline Memory* system_memory ();

}

#endif
