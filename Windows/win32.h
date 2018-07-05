// Nirvana project.
// Windows OS implementation.
// win32.h - base definitions for Win32 API.

#ifndef NIRVANA_WINDOWS_WIN32_H_
#define NIRVANA_WINDOWS_WIN32_H_

#define NOMINMAX
#include <windows.h>

// "interface" is used as identifier in ORB.
#ifdef interface
#undef interface
#endif

namespace Nirvana {
namespace Windows {

const SIZE_T PAGE_SIZE = 4096;
const SIZE_T PAGES_PER_BLOCK = 16; // Windows allocate memory by 64K blocks
const SIZE_T ALLOCATION_GRANULARITY = PAGE_SIZE * PAGES_PER_BLOCK;

}
}

#endif  //  _WIN32_H_
