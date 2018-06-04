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
	size.QuadPart = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY * sizeof (VirtualBlock);
	m_directory_size = (size_t)(size.QuadPart / sizeof (VirtualBlock));

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
	m_directory = (VirtualBlock**)VirtualAlloc (0, (size.QuadPart + ALLOCATION_GRANULARITY - 1) / ALLOCATION_GRANULARITY * sizeof (VirtualBlock*), MEM_RESERVE, PAGE_READWRITE);
#else
	m_directory = (VirtualBlock*)MapViewOfFile (m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
	if (!m_directory)
		throw INITIALIZE ();
}

void ProcessMemory::terminate ()
{
	if (m_directory) {
#ifdef _WIN64
		VirtualBlock** end = m_directory + (m_directory_size * sizeof (VirtualBlock) + ALLOCATION_GRANULARITY - 1) / ALLOCATION_GRANULARITY;
		for (VirtualBlock** page = m_directory; page < end; page += PAGE_SIZE / sizeof (VirtualBlock**)) {
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (page, &mbi, sizeof (mbi)));
			if (mbi.State == MEM_COMMIT) {
				VirtualBlock** end = page + PAGE_SIZE / sizeof (VirtualBlock**);
				for (VirtualBlock** p = page; p < end; ++p) {
					VirtualBlock* block = *p;
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

ProcessMemory::VirtualBlock& ProcessMemory::block (void* address)
{
	size_t idx = (size_t)address / ALLOCATION_GRANULARITY;
	assert (idx < m_directory_size);
	VirtualBlock* p;
#ifdef _WIN64
	size_t i0 = idx / SECOND_LEVEL_BLOCK;
	size_t i1 = idx % SECOND_LEVEL_BLOCK;
	if (!VirtualAlloc (m_directory + i0, sizeof (VirtualBlock*), MEM_COMMIT, PAGE_READWRITE))
		throw NO_MEMORY ();
	VirtualBlock** pp = m_directory + i0;
	p = *pp;
	if (!p) {
		LARGE_INTEGER offset;
		offset.QuadPart = ALLOCATION_GRANULARITY * i0;
		p = (VirtualBlock*)MapViewOfFile (m_mapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, ALLOCATION_GRANULARITY);
		if (!p)
			throw NO_MEMORY ();
		VirtualBlock* cur = (VirtualBlock*)InterlockedCompareExchangePointer ((void* volatile*)pp, p, 0);
		if (cur) {
			UnmapViewOfFile (p);
			p = cur;
		}
	}
	p += i1;
#else
	p = m_directory + idx;
#endif
	if (!VirtualAlloc (p, sizeof (VirtualBlock), MEM_COMMIT, PAGE_READWRITE))
		throw NO_MEMORY ();
	return *p;
}

void* ProcessMemory::reserve (size_t size, LONG flags, void* dst)
{
	if (!size)
		throw BAD_PARAM ();

	BYTE* p;
  if (dst && !(flags & Memory::EXACTLY))
    dst = ROUND_DOWN(dst, ALLOCATION_GRANULARITY);
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
		for (; pb < end; pb += ALLOCATION_GRANULARITY) {
			if (InterlockedCompareExchangePointer (&block (pb).mapping, INVALID_HANDLE_VALUE, 0))
				break;
		}
		if (pb >= end)
			break;
		while (pb > p)
			block (pb -= ALLOCATION_GRANULARITY).mapping = 0;
		verify (VirtualFreeEx (m_process, p, 0, MEM_RELEASE));
		raise_condition ();
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

	BYTE* begin = ROUND_DOWN((BYTE*)dst, ALLOCATION_GRANULARITY);
	BYTE* end = ROUND_UP((BYTE*)dst + size, ALLOCATION_GRANULARITY);

	// If memory is allocated by this service, mepping must be != 0.
	try {
		for (const BYTE* p = begin; p != end; p += ALLOCATION_GRANULARITY)
			if (!block (p).mapping)
				throw BAD_PARAM ();
	} catch (...) {
		throw BAD_PARAM ();
	}

	{
		// Define allocation margins if memory is reserved.
		MEMORY_BASIC_INFORMATION begin_mbi = {0}, end_mbi = {0};
		if (INVALID_HANDLE_VALUE == block ((const void*)begin).mapping) {
			verify (VirtualQueryEx (m_process, begin, &begin_mbi, sizeof (begin_mbi)));
			assert (MEM_RESERVE == begin_mbi.State);
			if ((BYTE*)begin_mbi.BaseAddress + begin_mbi.RegionSize >= end)
				end_mbi = begin_mbi;
		}

		if (!end_mbi.BaseAddress) {
			BYTE* back = end - PAGE_SIZE;
			if (INVALID_HANDLE_VALUE == block ((const void*)back).mapping) {
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
					raise_condition ();
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
					raise_condition ();
				}
			}
		}
	}

	// Release memory
	for (BYTE* p = begin; p < end;) {
		HANDLE mapping = InterlockedExchangePointer (&block (p).mapping, 0);
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
			p += ALLOCATION_GRANULARITY;
			while (p < region_end) {
				assert (INVALID_HANDLE_VALUE == block (p).mapping);
				block (p).mapping = 0;
				p += ALLOCATION_GRANULARITY;
			}
		} else {
			verify (UnmapViewOfFile2 (m_process, p, 0));
			verify (CloseHandle (mapping));
			p += ALLOCATION_GRANULARITY;
		}
	}
}

void ProcessMemory::map (void* dst, HANDLE src)
{
	if (!dst)
		throw BAD_PARAM ();
	dst = ROUND_DOWN (dst, ALLOCATION_GRANULARITY);

	VirtualBlock& vl = block (dst);
	if (!vl.mapping)	// Memory must be allocated
		throw BAD_PARAM ();

	HANDLE mapping = 0;
	if (!src) {
		if (vl.mapping != INVALID_HANDLE_VALUE)
			return;
		if (!(mapping = CreateFileMapping (0, 0, PAGE_READWRITE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0)))
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
			ptrdiff_t realloc_end = (BYTE*)mbi.BaseAddress + mbi.RegionSize - (BYTE*)dst - ALLOCATION_GRANULARITY;
			verify (VirtualFreeEx (m_process, dst, 0, MEM_RELEASE));
			if (realloc_begin > 0) {
				while (!VirtualAllocEx (m_process, mbi.AllocationBase, realloc_begin, MEM_RESERVE, mbi.AllocationProtect)) {
					assert (ERROR_INVALID_ADDRESS == GetLastError ());
					raise_condition ();
				}
			}
			if (realloc_end > 0) {
				BYTE* end = (BYTE*)dst + ALLOCATION_GRANULARITY;
				while (!VirtualAllocEx (m_process, end, realloc_end, MEM_RESERVE, mbi.AllocationProtect)) {
					assert (ERROR_INVALID_ADDRESS == GetLastError ());
					raise_condition ();
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
		while (!MapViewOfFile2 (mapping, m_process, 0, dst, ALLOCATION_GRANULARITY, 0, protect)) {
			assert (ERROR_INVALID_ADDRESS == GetLastError ());
			raise_condition ();
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
	dst = ROUND_DOWN (dst, ALLOCATION_GRANULARITY);
	VirtualBlock& vl = block (dst);
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
		while (!VirtualAllocEx (m_process, dst, ALLOCATION_GRANULARITY, MEM_RESERVE, mbi.AllocationProtect)) {
			assert (ERROR_INVALID_ADDRESS == GetLastError ());
			raise_condition ();
		}
	}
}

}
