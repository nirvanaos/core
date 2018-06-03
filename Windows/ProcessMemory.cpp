#include "ProcessMemory.h"
#include <Memory.h>
#include <cpplib.h>

namespace Nirvana {

using namespace std;

void ProcessMemory::initialize (DWORD process_id, HANDLE process_handle)
{
	m_process = process_handle;

	WCHAR name [22];
	wsprintfW (name, L"Nirvana.mmap.%08X", process_id);

	LARGE_INTEGER size;
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	size.QuadPart = ((size_t)si.lpMaximumApplicationAddress + LINE_SIZE) / LINE_SIZE * sizeof (VirtualLine);
	m_directory_size = (size_t)(size.QuadPart / sizeof (VirtualLine));

	if (GetCurrentProcessId () == process_id) {

#ifndef NDEBUG
		_set_error_mode (_OUT_TO_MSGBOX);
#endif

		m_mapping = CreateFileMapping (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, name);
		if (!m_mapping)
			throw INITIALIZE ();
	} else {
		m_mapping = OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, name);
		if (!m_mapping)
			throw INITIALIZE ();
	}

#ifdef _WIN64
	m_directory = (VirtualLine**)VirtualAlloc (0, (size.QuadPart + LINE_SIZE - 1) / LINE_SIZE * sizeof (VirtualLine*), MEM_RESERVE, PAGE_READWRITE);
#else
	m_directory = (VirtualLine*)MapViewOfFile (m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
	if (!m_directory)
		throw INITIALIZE ();
}

void ProcessMemory::terminate ()
{
	if (m_directory) {
#ifdef _WIN64
		VirtualLine** end = m_directory + (m_directory_size * sizeof (VirtualLine) + LINE_SIZE - 1) / LINE_SIZE;
		for (VirtualLine** page = m_directory; page < end; page += PAGE_SIZE / sizeof (VirtualLine**)) {
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (page, &mbi, sizeof (mbi)));
			if (mbi.State == MEM_COMMIT) {
				VirtualLine** end = page + PAGE_SIZE / sizeof (VirtualLine**);
				for (VirtualLine** p = page; p < end; ++p) {
					VirtualLine* block = *p;
					if (block)
						verify (UnmapViewOfFile (block));
				}
			}
		}
		verify (VirtualFree (m_directory, 0, MEM_RELEASE));
#else
		verify (UnmapViewOfFile (m_directory));
#endif
		m_directory = 0;
	}
	if (m_mapping) {
		verify (CloseHandle (m_mapping));
		m_mapping = 0;
	}
}

ProcessMemory::VirtualLine& ProcessMemory::line (void* address)
{
	size_t idx = (size_t)address / LINE_SIZE;
	assert (idx < m_directory_size);
	VirtualLine* p;
#ifdef _WIN64
	size_t i0 = idx / SECOND_LEVEL_BLOCK;
	size_t i1 = idx % SECOND_LEVEL_BLOCK;
	if (!VirtualAlloc (m_directory + i0, sizeof (VirtualLine*), MEM_COMMIT, PAGE_READWRITE))
		throw NO_MEMORY ();
	p = m_directory [i0];
	if (!p) {
		LARGE_INTEGER offset;
		offset.QuadPart = LINE_SIZE * i0;
		p = (VirtualLine*)MapViewOfFile (m_mapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, LINE_SIZE);
		if (!p)
			throw NO_MEMORY ();
		m_directory [i0] = p;
	}
	p += i1;
#else
	p = m_directory + idx;
#endif
	if (!VirtualAlloc (p, sizeof (VirtualLine), MEM_COMMIT, PAGE_READWRITE))
		throw NO_MEMORY ();
	return *p;
}

void* ProcessMemory::reserve (size_t size, LONG flags, void* dst)
{
	if (!size)
		throw BAD_PARAM ();

	BYTE* p;
  if (dst && !(flags & Memory::EXACTLY))
    dst = ROUND_DOWN(dst, LINE_SIZE);
	for (;;) {	// Loop to handle possible raise conditions.
		p = (BYTE*)VirtualAllocEx (m_process, dst, size, MEM_RESERVE, (flags & Memory::READ_ONLY) ? WIN_READ_RESERVE : WIN_WRITE_RESERVE);
		if (!p) {
			if (dst && (flags & Memory::EXACTLY))
				return 0;
			else
				throw NO_MEMORY ();
		}

		BYTE* pb = p;
		BYTE* end = p + size;
		for (; pb < end; pb += LINE_SIZE) {
			if (InterlockedCompareExchangePointer (&line (pb).mapping, INVALID_HANDLE_VALUE, 0))
				break;
		}
		if (pb >= end)
			break;
		while (pb > p)
			line (pb -= LINE_SIZE).mapping = 0;
		verify (VirtualFreeEx (m_process, p, 0, MEM_RELEASE));
		Sleep (0);
	}
	if (dst && (flags & Memory::EXACTLY))
		return dst;
	else
		return p;
}

void ProcessMemory::release (void* dst, size_t size)
{
	if (!(dst && size))
		return;

	BYTE* begin = ROUND_DOWN((BYTE*)dst, LINE_SIZE);
	BYTE* end = ROUND_UP((BYTE*)dst + size, LINE_SIZE);

	// If memory is allocated by this service, mepping must be != 0.
	try {
		for (const BYTE* p = begin; p != end; p += LINE_SIZE)
			if (!line (p).mapping)
				throw BAD_PARAM ();
	} catch (...) {
		throw BAD_PARAM ();
	}

	{
		// Define allocation margins if memory is reserved.
		MEMORY_BASIC_INFORMATION begin_mbi = {0}, end_mbi = {0};
		if (INVALID_HANDLE_VALUE == line ((const void*)begin).mapping) {
			verify (VirtualQueryEx (m_process, begin, &begin_mbi, sizeof (begin_mbi)));
			assert (MEM_RESERVE == begin_mbi.State);
			if ((BYTE*)begin_mbi.BaseAddress + begin_mbi.RegionSize >= end)
				end_mbi = begin_mbi;
		}

		if (!end_mbi.BaseAddress) {
			BYTE* back = end - PAGE_SIZE;
			if (INVALID_HANDLE_VALUE == line ((const void*)back).mapping) {
				verify (VirtualQueryEx (m_process, back, &end_mbi, sizeof (end_mbi)));
				assert (MEM_RESERVE == end_mbi.State);
			}
		}

		// Split reserved blocks at begin and end if need.
		if (begin_mbi.BaseAddress) {
			ptrdiff_t realloc = begin - (BYTE*)begin_mbi.AllocationBase;
			if (realloc > 0) {
				verify (VirtualFreeEx (m_process, begin_mbi.AllocationBase, 0, MEM_RELEASE));
				while (!VirtualAllocEx (m_process, begin_mbi.AllocationBase, realloc, MEM_RESERVE, begin_mbi.AllocationProtect)) {
					assert (ERROR_INVALID_ADDRESS == GetLastError ());
					Sleep (0);
				}
			}
		}

		if (end_mbi.BaseAddress) {
			ptrdiff_t realloc = (BYTE*)end_mbi.BaseAddress + end_mbi.RegionSize - end;
			if (realloc > 0) {
				if ((BYTE*)end_mbi.AllocationBase >= begin)
					verify (VirtualFreeEx (m_process, end_mbi.AllocationBase, 0, MEM_RELEASE));
				while (!VirtualAllocEx (m_process, end, realloc, MEM_RESERVE, end_mbi.AllocationProtect)) {
					assert (ERROR_INVALID_ADDRESS == GetLastError ());
					Sleep (0);
				}
			}
		}
	}

	// Release memory
	for (BYTE* p = begin; p < end;) {
		HANDLE mapping = InterlockedExchangePointer (&line (p).mapping, 0);
		if (INVALID_HANDLE_VALUE == mapping) {
			MEMORY_BASIC_INFORMATION mbi;
			if (!VirtualQueryEx (m_process, p, &mbi, sizeof (mbi)))
				throw INTERNAL ();
			assert (mbi.State == MEM_RESERVE || mbi.State == MEM_FREE);
			if (mbi.State == MEM_RESERVE)
				verify (VirtualFreeEx (m_process, p, 0, MEM_RELEASE));
			BYTE* region_end = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
			if (region_end > end)
				region_end = end;
			p += LINE_SIZE;
			while (p < region_end) {
				assert (INVALID_HANDLE_VALUE == line (p).mapping);
				line (p).mapping = 0;
				p += LINE_SIZE;
			}
		} else {
			verify (UnmapViewOfFile2 (m_process, p, 0));
			verify (CloseHandle (mapping));
			p += LINE_SIZE;
		}
	}
}

void ProcessMemory::map (void* dst, HANDLE src)
{
	if (!dst)
		throw BAD_PARAM ();
	dst = ROUND_DOWN (dst, LINE_SIZE);

	VirtualLine& vl = line (dst);
	if (!vl.mapping)	// Memory must be allocated
		throw BAD_PARAM ();

	HANDLE mapping = 0;
	if (!src) {
		if (vl.mapping != INVALID_HANDLE_VALUE)
			return;
		if (!(mapping = CreateFileMapping (0, 0, PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0)))
			throw NO_MEMORY ();
	} else {
		if (!DuplicateHandle (GetCurrentProcess (), src, m_process, &mapping, 0, FALSE, DUPLICATE_SAME_ACCESS))
			throw NO_MEMORY ();
	}

	try {
		MEMORY_BASIC_INFORMATION mbi;
		verify (VirtualQueryEx (m_process, dst, &mbi, sizeof (mbi)));
		if (!src && !(mbi.AllocationProtect & WIN_MASK_WRITE))
			throw BAD_PARAM ();
		HANDLE old = InterlockedExchangePointer (&vl.mapping, mapping);
		if (old == INVALID_HANDLE_VALUE) {
			assert (MEM_RESERVE == mbi.State);
			ptrdiff_t realloc_begin = (BYTE*)dst - (BYTE*)mbi.AllocationBase;
			ptrdiff_t realloc_end = (BYTE*)mbi.BaseAddress + mbi.RegionSize - (BYTE*)dst - LINE_SIZE;
			verify (VirtualFreeEx (m_process, dst, 0, MEM_RELEASE));
			if (realloc_begin > 0) {
				while (!VirtualAllocEx (m_process, mbi.AllocationBase, realloc_begin, MEM_RESERVE, mbi.AllocationProtect)) {
					assert (ERROR_INVALID_ADDRESS == GetLastError ());
					Sleep (0);
				}
			}
			if (realloc_end > 0) {
				BYTE* end = (BYTE*)dst + LINE_SIZE;
				while (!VirtualAllocEx (m_process, end, realloc_end, MEM_RESERVE, mbi.AllocationProtect)) {
					assert (ERROR_INVALID_ADDRESS == GetLastError ());
					Sleep (0);
				}
			}
		} else if (old) {
			if (!UnmapViewOfFile2 (m_process, dst, 0))
				throw INTERNAL ();
			verify (CloseHandle (old));
		} else {
			vl.mapping = 0;
			throw BAD_PARAM ();
		}

		ULONG protect = (mbi.AllocationProtect & WIN_MASK_WRITE) ? (src ? WIN_WRITE_MAPPED_SHARED : WIN_WRITE_MAPPED_PRIVATE) : (src ? WIN_READ_COPIED : WIN_READ_MAPPED);
		while (!MapViewOfFile2 (mapping, m_process, 0, dst, LINE_SIZE, 0, protect)) {
			assert (ERROR_INVALID_ADDRESS == GetLastError ());
			Sleep (0);
		}

	} catch (...) {
		CloseHandle (mapping);
		throw;
	}
}

void ProcessMemory::unmap (void* dst)
{
	if (!dst)
		throw BAD_PARAM ();
	dst = ROUND_DOWN (dst, LINE_SIZE);
	VirtualLine& vl = line (dst);
	HANDLE mapping = InterlockedExchangePointer (&vl.mapping, INVALID_HANDLE_VALUE);
	if (!mapping) {
		vl.mapping = 0;
		throw BAD_PARAM ();
	}
	if (INVALID_HANDLE_VALUE != mapping) {
		MEMORY_BASIC_INFORMATION mbi;
		verify (VirtualQueryEx (m_process, dst, &mbi, sizeof (mbi)));
		verify (UnmapViewOfFile2 (m_process, dst, 0));
		verify (CloseHandle (mapping));
		while (!VirtualAllocEx (m_process, dst, LINE_SIZE, MEM_RESERVE, mbi.AllocationProtect)) {
			assert (ERROR_INVALID_ADDRESS == GetLastError ());
			Sleep (0);
		}
	}
}

}
