// Nirvana project.
// Windows OS implementation.
// win32.h - base definitions for Win32 API.

#ifndef NIRVANA_WINDOWS_WIN32_H_
#define NIRVANA_WINDOWS_WIN32_H_

#if !(defined (_M_IX86) || defined (_M_AMD64))
#error Only x86 and x64 platforms supported for Windows.
#endif

#include "../core.h"
#define NOMINMAX
#include <windows.h>

#if (FIXED_PROTECTION_UNIT && (FIXED_PROTECTION_UNIT != 4096))
#error Windows version assumes page size 4096 bytes.
#endif

// "interface" is used as identifier in ORB.
#ifdef interface
#undef interface
#endif

namespace Nirvana {
namespace Windows {

const UWord PAGE_SIZE = 4096;
const UWord PAGES_PER_BLOCK = 16; // Windows allocate memory by 64K blocks
const UWord ALLOCATION_GRANULARITY = PAGE_SIZE * PAGES_PER_BLOCK;

}
}

#endif  //  _WIN32_H_
