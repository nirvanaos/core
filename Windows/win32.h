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

// Thread information block
static const NT_TIB* current_TIB ()
{
#ifdef _M_IX86
	return (const NT_TIB*)__readfsdword (0x18);
#elif _M_AMD64
	return (const NT_TIB*)__readgsqword (0x30);
#else
#error Only x86 and x64 platforms supported.
#endif
}

// Helper to define is memory
inline bool is_current_stack (void* p)
{
	void* stack_begin = &stack_begin;
	return p >= stack_begin && p < current_TIB ()->StackBase;
}

}
}

#endif  //  _WIN32_H_
