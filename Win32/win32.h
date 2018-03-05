// Nirvana Emulation for Win32
// win32.h - base definitions

#ifndef _WIN32_H_
#define _WIN32_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _M_IX86
#error Only Intel platform supported for Windows.
#endif

#include <core.h>
#define _WIN32_WINNT 0x0400
#define UNICODE
#include <windows.h>

using namespace Nirvana;

const UWord PAGE_SIZE = 4096;

#if (FIXED_PROTECTION_UNIT && (FIXED_PROTECTION_UNIT != 4096))
#error Windows version assumes page size 4096 bytes.
#endif

struct Page {
  Octet bytes [PAGE_SIZE];

  static Page* begin (void* p)
  {
    return (Page*)((UWord)p & ~(PAGE_SIZE - 1));
  }
  
  static const Page* begin (const void* p)
  {
    return (const Page*)((UWord)p & ~(PAGE_SIZE - 1));
  }
  
  static Page* end (void* p)
  {
    return (Page*)(((UWord)p + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
  }
  
  static const Page* end (const void* p)
  {
    return (const Page*)(((UWord)p + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
  }
};

const UWord PAGES_PER_LINE = 16; // Windows allocate memory by 64K blocks
const UWord LINE_SIZE = PAGE_SIZE * PAGES_PER_LINE;

struct Line {
  Page pages [PAGES_PER_LINE];
  
  static Line* begin (void* p)
  {
    return (Line*)((UWord)p & ~(LINE_SIZE - 1));
  }
  
  static const Line* begin (const void* p)
  {
    return (const Line*)((UWord)p & ~(LINE_SIZE - 1));
  }
  
  static Line* end (void* p)
  {
    return (Line*)(((UWord)p + LINE_SIZE - 1) & ~(LINE_SIZE - 1));
  }
  
  static const Line* end (const void* p)
  {
    return (const Line*)(((UWord)p + LINE_SIZE - 1) & ~(LINE_SIZE - 1));
  }
};

#endif  //  _WIN32_H_
