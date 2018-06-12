#include "AddressSpace.h"
#include <Memory.h>
#include <cpplib.h>

namespace Nirvana {
namespace Windows {

using namespace ::CORBA;

AddressSpace::Block::Block (AddressSpace& space, void* address) :
	m_space (space),
	m_address (round_down ((BYTE*)address, ALLOCATION_GRANULARITY)),
	m_info (*space.allocated_block (address))
{
	if (!&m_info)
		throw BAD_PARAM ();
}

const AddressSpace::Block::State& AddressSpace::Block::state ()
{
	if (State::INVALID == m_state.state) {
		MEMORY_BASIC_INFORMATION mbi;
		m_space.query (m_address, mbi);
		DWORD page_state_bits = mbi.Protect;
		if (mbi.Type == MEM_MAPPED) {
			assert (mapping () != INVALID_HANDLE_VALUE);
			assert (mbi.AllocationBase == m_address);
			m_state.state = State::MAPPED;
			BYTE* page = m_address;
			BYTE* block_end = page + ALLOCATION_GRANULARITY;
			auto* ps = m_state.mapped.page_state;
			for (;;) {
				BYTE* end = page + mbi.RegionSize;
				assert (end <= block_end);
				page_state_bits |= mbi.Protect;
				for (; page < end; page += PAGE_SIZE) {
					*(ps++) = mbi.Protect;
				}
				if (end < block_end)
					m_space.query (end, mbi);
				else
					break;
			}
		} else {
			assert (mapping () == INVALID_HANDLE_VALUE);
			assert ((BYTE*)mbi.BaseAddress + mbi.RegionSize >= (m_address + ALLOCATION_GRANULARITY));
			m_state.state = State::RESERVED;

			m_state.reserved.begin = (BYTE*)mbi.AllocationBase;
			m_state.reserved.end = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
		}
		m_state.page_state_bits = page_state_bits;
	}
	return m_state;
}

void AddressSpace::Block::map (HANDLE mapping, MappingType protection)
{
	assert (mapping);

	MEMORY_BASIC_INFORMATION mbi;
	m_space.query (m_address, mbi);
	HANDLE old = InterlockedExchangePointer (&m_info.mapping, mapping);
	if (old == INVALID_HANDLE_VALUE) {
		const State& bs = state ();
		assert (State::RESERVED == bs.state);
		BYTE* reserved_begin = bs.reserved.begin;
		ptrdiff_t realloc_begin = m_address - reserved_begin;
		ptrdiff_t realloc_end = bs.reserved.end - m_address - ALLOCATION_GRANULARITY;
		verify (VirtualFreeEx (m_space.process (), reserved_begin, 0, MEM_RELEASE));
		if (realloc_begin > 0) {
			while (!VirtualAllocEx (m_space.process (), reserved_begin, realloc_begin, MEM_RESERVE, mbi.AllocationProtect)) {
				assert (ERROR_INVALID_ADDRESS == GetLastError ());
				raise_condition ();
			}
		}
		if (realloc_end > 0) {
			BYTE* end = m_address + ALLOCATION_GRANULARITY;
			while (!VirtualAllocEx (m_space.process (), end, realloc_end, MEM_RESERVE, mbi.AllocationProtect)) {
				assert (ERROR_INVALID_ADDRESS == GetLastError ());
				raise_condition ();
			}
		}
	} else if (old) {
		verify (UnmapViewOfFile2 (m_space.process (), m_address, 0));
		verify (CloseHandle (old));
	} else {
		m_info.mapping = 0;
		throw INTERNAL ();
	}

	while (!MapViewOfFile2 (mapping, m_space.process (), 0, m_address, ALLOCATION_GRANULARITY, 0, protection)) {
		assert (ERROR_INVALID_ADDRESS == GetLastError ());
		raise_condition ();
	}
}

void AddressSpace::Block::unmap ()
{
	HANDLE mapping = InterlockedExchangePointer (&m_info.mapping, INVALID_HANDLE_VALUE);
	if (!mapping) {
		m_info.mapping = 0;
		throw INTERNAL ();
	}
	if (INVALID_HANDLE_VALUE != mapping) {
		verify (UnmapViewOfFile2 (m_space.process (), m_address, 0));
		verify (CloseHandle (mapping));
		while (!VirtualAllocEx (m_space.process (), m_address, ALLOCATION_GRANULARITY, MEM_RESERVE, PAGE_NOACCESS)) {
			assert (ERROR_INVALID_ADDRESS == GetLastError ());
			raise_condition ();
		}
	}
}

bool AddressSpace::Block::has_data_outside_of (SIZE_T offset, SIZE_T size)
{
	SIZE_T offset_end = offset + size;
	assert (offset_end <= ALLOCATION_GRANULARITY);
	if (offset || size < ALLOCATION_GRANULARITY) {
		auto page_state = state ().mapped.page_state;
		if (offset) {
			for (auto ps = page_state, end = page_state + (offset + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps) {
				if (PageState::MASK_ACCESS & *ps)
					return true;
			}
		}
		if (offset_end < ALLOCATION_GRANULARITY) {
			for (auto ps = page_state + (offset_end) / PAGE_SIZE, end = page_state + PAGES_PER_BLOCK; ps < end; ++ps) {
				if (PageState::MASK_ACCESS & *ps)
					return true;
			}
		}
	}
}

void AddressSpace::Block::copy (HANDLE src, SIZE_T offset, SIZE_T size, LONG flags)
{
	assert (size);
	SIZE_T offset_end = offset + size;
	assert (offset_end <= ALLOCATION_GRANULARITY);

	bool remap;
	HANDLE cur_mapping = mapping ();
	if (INVALID_HANDLE_VALUE == cur_mapping)
		remap = true;
	else if (!CompareObjectHandles (cur_mapping, src)) {
		// Change mapping, if possible
		if (has_data_outside_of (offset, size))
			throw INTERNAL ();
		remap = true;
	}

	if (remap) {
		HANDLE mapping;
		if (!DuplicateHandle (GetCurrentProcess (), src, m_space.process (), &mapping, 0, FALSE, DUPLICATE_SAME_ACCESS))
			throw NO_MEMORY ();
		try {
			map (src, MAP_SHARED);
		} catch (...) {
			CloseHandle (mapping);
			throw;
		}
		if (Memory::READ_ONLY & flags)
			m_space.protect (m_address + offset, size, PageState::RO_MAPPED_SHARED);
		if (offset > 0)
			m_space.protect (m_address, round_down (offset, PAGE_SIZE), PageState::DECOMMITTED);
		offset = round_up (offset + size, PAGE_SIZE);
		SIZE_T tail = ALLOCATION_GRANULARITY - offset;
		if (tail > 0)
			m_space.protect (m_address + offset, tail, PageState::DECOMMITTED);
	} else {
		bool change_protection = false;
		DWORD protection = (Memory::READ_ONLY & flags) ? PageState::RO_MAPPED_SHARED | PAGE_REVERT_TO_FILE_MAP : PageState::RW_MAPPED_SHARED | PAGE_REVERT_TO_FILE_MAP;
		auto page_state = state ().mapped.page_state;
		auto begin = page_state + offset / PAGE_SIZE, end = page_state + (offset_end + PAGE_SIZE - 1) / PAGE_SIZE - 1;
		if (offset % PAGE_SIZE) {	// Destination address is not page-aligned.
			auto state = *begin;
			if (state != protection) {
				if (PageState::MASK_UNMAPPED & state)
					throw INTERNAL ();	// Can not copy unaligned data to unmapped private page.
				else if (PageState::NOT_COMMITTED == state)
					throw BAD_PARAM ();	// Can not copy not committed page - source error.
				else if (PageState::DECOMMITTED == state)
					change_protection = true;
				else if (Memory::READ_ONLY & flags)
					offset = round_up (offset, PAGE_SIZE); // Start page should remain partially writable.
			}
			++begin;
		}

		{
			SIZE_T tail = offset_end % PAGE_SIZE;
			if (tail) {	// End of destination block is not page-aligned
				auto state = *end;
				if (state != protection) {
					if (PageState::MASK_UNMAPPED & state)
						throw INTERNAL ();	// Can not copy unaligned data to unmapped private page.
					else if (PageState::NOT_COMMITTED == state)
						throw BAD_PARAM ();	// Can not copy not committed page - source error.
					else if (PageState::DECOMMITTED == state)
						change_protection = true;
					else if (Memory::READ_ONLY & flags)
						size -= tail; // End page should remain partially writable.
				}
			}
			--end;
		}

		for (auto ps = begin; ps <= end; ++ps) {
			auto state = *ps;
			if (state != protection) {
				if (PageState::NOT_COMMITTED == state)
					throw BAD_PARAM ();	// Can not copy not committed page
				else
					change_protection = true;
			}
		}
		if (change_protection) {
			m_space.protect (m_address + offset, size, protection | PAGE_REVERT_TO_FILE_MAP);
			invalidate_state ();
		}
	}
}

void AddressSpace::Block::change_protection (SIZE_T offset, SIZE_T size, LONG flags)
{
	SIZE_T offset_end = offset + size;
	assert (offset_end <= ALLOCATION_GRANULARITY);
	assert (size);

	static const SIZE_T STATES_CNT = 3;

	static const DWORD states_RW [STATES_CNT] = {
		PageState::RW_MAPPED_PRIVATE,
		PageState::RW_MAPPED_SHARED,
		PageState::RW_UNMAPPED
	};

	static const DWORD states_RO [STATES_CNT] = {
		PageState::RO_MAPPED_PRIVATE,
		PageState::RO_MAPPED_SHARED,
		PageState::RO_UNMAPPED
	};

	DWORD protect_mask;
	const DWORD* states_src;
	const DWORD* states_dst;

	if (flags & Memory::READ_ONLY) {
		protect_mask = PageState::MASK_RO;
		states_src = states_RW;
		states_dst = states_RO;
		offset = round_up (offset, PAGE_SIZE);
		offset_end = round_down (offset_end, PAGE_SIZE);
	} else {
		protect_mask = PageState::MASK_RW;
		states_src = states_RO;
		states_dst = states_RW;
		offset = round_down (offset, PAGE_SIZE);
		offset_end = round_up (offset_end, PAGE_SIZE);
	}

	auto page_state = state ().mapped.page_state;
	auto region_begin = page_state + offset / PAGE_SIZE, state_end = page_state + offset_end / PAGE_SIZE;
	do {
		auto region_end = region_begin;
		DWORD state = *region_begin;
		do
			++region_end;
		while (region_end < state_end && state == *region_end);

		if (!(protect_mask & state)) {

			DWORD new_state = state;
			for (SIZE_T i = 0; i < STATES_CNT; ++i) {
				if (states_src [i] == state) {
					new_state = states_dst [i];
					break;
				}
			}

			if (new_state != state) {
				BYTE* ptr = address () + (region_begin - page_state) * PAGE_SIZE;
				SIZE_T size = (region_end - region_begin) * PAGE_SIZE;
				m_space.protect (ptr, size, new_state);
			}
		}

		region_begin = region_end;
	} while (region_begin < state_end);
}

DWORD AddressSpace::Block::check_committed (SIZE_T offset, SIZE_T size)
{
	assert (offset + size <= ALLOCATION_GRANULARITY);

	const State& bs = state ();
	if (bs.state != State::MAPPED)
		throw BAD_PARAM ();
	for (auto ps = bs.mapped.page_state + offset / PAGE_SIZE, end = bs.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps)
		if (!(PageState::MASK_ACCESS & *ps))
			throw BAD_PARAM ();
	return bs.page_state_bits;
}

void AddressSpace::Block::decommit (SIZE_T offset, SIZE_T size)
{
	offset = round_up (offset, PAGE_SIZE);
	SIZE_T offset_end = round_down (offset + size, PAGE_SIZE);
	assert (offset_end <= ALLOCATION_GRANULARITY);
	if (offset < offset_end) {
		if (!offset && offset_end == ALLOCATION_GRANULARITY)
			unmap ();
		else if (state ().state == State::MAPPED) {
			bool can_unmap = true;
			auto page_state = state ().mapped.page_state;
			if (offset) {
				for (auto ps = page_state, end = page_state + offset / PAGE_SIZE; ps < end; ++ps) {
					if (PageState::MASK_ACCESS & *ps) {
						can_unmap = false;
						break;
					}
				}
			}
			if (can_unmap && offset_end < ALLOCATION_GRANULARITY) {
				for (auto ps = page_state + offset_end / PAGE_SIZE, end = page_state + PAGES_PER_BLOCK; ps < end; ++ps) {
					if (PageState::MASK_ACCESS & *ps) {
						can_unmap = false;
						break;
					}
				}
			}

			if (can_unmap)
				unmap ();
			else {
				// Decommit pages. We can't use VirtualFree and MEM_DECOMMIT with mapped memory.
				m_space.protect (address () + offset, offset_end - offset, PageState::DECOMMITTED | PAGE_REVERT_TO_FILE_MAP);

				// Discard private pages.
				auto region_begin = page_state + offset / PAGE_SIZE, state_end = page_state + offset_end / PAGE_SIZE;
				do {
					auto region_end = region_begin;
					if (!((PageState::MASK_MAY_BE_SHARED | PageState::DECOMMITTED) & *region_begin)) {
						do
							++region_end;
						while (region_end < state_end && !((PageState::MASK_MAY_BE_SHARED | PageState::DECOMMITTED) & *region_end));

						BYTE* ptr = address () + (region_begin - page_state) * PAGE_SIZE;
						SIZE_T size = (region_end - region_begin) * PAGE_SIZE;
						verify (VirtualAllocEx (m_space.process (), ptr, size, MEM_RESET, PageState::DECOMMITTED));
					} else {
						do
							++region_end;
						while (region_end < state_end && ((PageState::MASK_MAY_BE_SHARED | PageState::DECOMMITTED) & *region_end));
					}
					region_begin = region_end;
				} while (region_begin < state_end);

				// Invalidate block state.
				invalidate_state ();
			}
		}
	}
}

void AddressSpace::initialize (DWORD process_id, HANDLE process_handle)
{
	m_process = process_handle;

	WCHAR name [22];
	wsprintfW (name, L"Nirvana.mmap.%08X", process_id);

	SYSTEM_INFO si;
	GetSystemInfo (&si);
	m_directory_size = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY;

	if (GetCurrentProcessId () == process_id) {

#ifndef NDEBUG
		_set_error_mode (_OUT_TO_MSGBOX);
#endif

		LARGE_INTEGER size;
		size.QuadPart = m_directory_size * sizeof (BlockInfo);
		m_mapping = CreateFileMapping (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, name);
		if (!m_mapping)
			throw INITIALIZE ();
	} else {
		m_mapping = OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, name);
		if (!m_mapping)
			throw INITIALIZE ();
	}

#ifdef _WIN64
	m_directory = (BlockInfo**)VirtualAlloc (0, (m_directory_size + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK * sizeof (BlockInfo*), MEM_RESERVE, PAGE_READWRITE);
#else
	m_directory = (BlockInfo*)MapViewOfFile (m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
	if (!m_directory)
		throw INITIALIZE ();
}

void AddressSpace::terminate ()
{
	if (m_directory) {
#ifdef _WIN64
		BlockInfo** end = m_directory + (m_directory_size + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK;
		for (BlockInfo** page = m_directory; page < end; page += PAGE_SIZE / sizeof (BlockInfo**)) {
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (page, &mbi, sizeof (mbi)));
			if (mbi.State == MEM_COMMIT) {
				BlockInfo** end = page + PAGE_SIZE / sizeof (BlockInfo**);
				for (BlockInfo** p = page; p < end; ++p) {
					BlockInfo* block = *p;
					if (block) {
#ifdef _DEBUG
						if (GetCurrentProcess () == m_process) {
							BYTE* address = (BYTE*)((p - m_directory) * SECOND_LEVEL_BLOCK * ALLOCATION_GRANULARITY);
							for (BlockInfo* page = block, *end = block + SECOND_LEVEL_BLOCK; page != end; page += PAGE_SIZE / sizeof (BlockInfo)) {
								verify (VirtualQuery (page, &mbi, sizeof (mbi)));
								if (mbi.State == MEM_COMMIT) {
									for (BlockInfo* p = page, *end = page + PAGE_SIZE / sizeof (BlockInfo); p != end; ++p, address += ALLOCATION_GRANULARITY) {
										HANDLE hm = p->mapping;
										if (INVALID_HANDLE_VALUE == hm) {
											VirtualFreeEx (m_process, address, 0, MEM_RELEASE);
										} else {
											if (hm) {
												UnmapViewOfFile2 (m_process, address, 0);
												CloseHandle (hm);
											}
										}
									}
								} else
									address += PAGE_SIZE / sizeof (BlockInfo) * ALLOCATION_GRANULARITY;
							}
						}
#endif
						verify (UnmapViewOfFile (block));
					}
				}
			}
		}
		verify (VirtualFree (m_directory, 0, MEM_RELEASE));
#else
#ifdef _DEBUG
		if (GetCurrentProcess () == m_process) {
			BYTE* address = 0;
			for (BlockInfo* page = m_directory, *end = m_directory + m_directory_size; page < end; page += PAGE_SIZE / sizeof (BlockInfo)) {
				MEMORY_BASIC_INFORMATION mbi;
				verify (VirtualQuery (page, &mbi, sizeof (mbi)));
				if (mbi.State == MEM_COMMIT) {
					for (BlockInfo* p = page, *end = page + PAGE_SIZE / sizeof (BlockInfo); p != end; ++p, address += ALLOCATION_GRANULARITY) {
						HANDLE hm = p->mapping;
						if (INVALID_HANDLE_VALUE == hm) {
							VirtualFreeEx (m_process, address, 0, MEM_RELEASE);
						} else {
							if (hm) {
								UnmapViewOfFile2 (m_process, address, 0);
								CloseHandle (hm);
							}
						}
					}
				} else
					address += PAGE_SIZE / sizeof (BlockInfo) * ALLOCATION_GRANULARITY;
			}
		}
#endif
		verify (UnmapViewOfFile (m_directory));
#endif
		m_directory = 0;
	}
	if (m_mapping) {
		verify (CloseHandle (m_mapping));
		m_mapping = 0;
	}
}

AddressSpace::BlockInfo& AddressSpace::block (void* address)
{
	size_t idx = (size_t)address / ALLOCATION_GRANULARITY;
	assert (idx < m_directory_size);
	BlockInfo* p;
#ifdef _WIN64
	size_t i0 = idx / SECOND_LEVEL_BLOCK;
	size_t i1 = idx % SECOND_LEVEL_BLOCK;
	if (!VirtualAlloc (m_directory + i0, sizeof (BlockInfo*), MEM_COMMIT, PAGE_READWRITE))
		throw NO_MEMORY ();
	BlockInfo** pp = m_directory + i0;
	p = *pp;
	if (!p) {
		LARGE_INTEGER offset;
		offset.QuadPart = ALLOCATION_GRANULARITY * i0;
		p = (BlockInfo*)MapViewOfFile (m_mapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, ALLOCATION_GRANULARITY);
		if (!p)
			throw NO_MEMORY ();
		BlockInfo* cur = (BlockInfo*)InterlockedCompareExchangePointer ((void* volatile*)pp, p, 0);
		if (cur) {
			UnmapViewOfFile (p);
			p = cur;
		}
	}
	p += i1;
#else
	p = m_directory + idx;
#endif
	if (!VirtualAlloc (p, sizeof (BlockInfo), MEM_COMMIT, PAGE_READWRITE))
		throw NO_MEMORY ();
	return *p;
}

AddressSpace::BlockInfo* AddressSpace::allocated_block (void* address)
{
	size_t idx = (size_t)address / ALLOCATION_GRANULARITY;
	BlockInfo* p = 0;
	if (idx < m_directory_size) {
#ifdef _WIN64
		size_t i0 = idx / SECOND_LEVEL_BLOCK;
		size_t i1 = idx % SECOND_LEVEL_BLOCK;
		BlockInfo** pp = m_directory + i0;
		if (!IsBadReadPtr (pp, sizeof (BlockInfo*)) {
			if (p = *pp)
				p += i1;
		}
#else
		p = m_directory + idx;
		if (IsBadReadPtr (p, sizeof (BlockInfo)))
			p = 0;
#endif
		if (p && !p->mapping)
			p = 0;
	}
	return p;
}

void* AddressSpace::reserve (SIZE_T size, LONG flags, void* dst)
{
	if (!size)
		throw BAD_PARAM ();

	BYTE* p;
	if (dst && !(flags & Memory::EXACTLY))
		dst = round_down (dst, ALLOCATION_GRANULARITY);
	size = round_up (size, ALLOCATION_GRANULARITY);
	for (;;) {	// Loop to handle possible raise conditions.
		p = (BYTE*)VirtualAllocEx (m_process, dst, size, MEM_RESERVE, PAGE_NOACCESS);
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

void AddressSpace::release (void* dst, SIZE_T size)
{
	if (!(dst && size))
		return;

	BYTE* begin = round_down ((BYTE*)dst, ALLOCATION_GRANULARITY);
	BYTE* end = round_up ((BYTE*)dst + size, ALLOCATION_GRANULARITY);

	// Check allocation.
	for (BYTE* p = begin; p != end; p += ALLOCATION_GRANULARITY)
		if (!allocated_block (p))
			throw BAD_PARAM ();

	{ // Define allocation margins if memory is reserved.
		MEMORY_BASIC_INFORMATION begin_mbi = {0}, end_mbi = {0};
		if (INVALID_HANDLE_VALUE == allocated_block (begin)->mapping) {
			query (begin, begin_mbi);
			assert (MEM_RESERVE == begin_mbi.State);
			if ((BYTE*)begin_mbi.BaseAddress + begin_mbi.RegionSize >= end)
				end_mbi = begin_mbi;
		}

		if (!end_mbi.BaseAddress) {
			BYTE* back = end - PAGE_SIZE;
			if (INVALID_HANDLE_VALUE == allocated_block (back)->mapping) {
				query (back, end_mbi);
				assert (MEM_RESERVE == end_mbi.State);
			}
		}

		// Split reserved blocks at begin and end if need.
		if (begin_mbi.BaseAddress) {
			SSIZE_T realloc = begin - (BYTE*)begin_mbi.AllocationBase;
			if (realloc > 0) {
				verify (VirtualFreeEx (m_process, begin_mbi.AllocationBase, 0, MEM_RELEASE));
				while (!VirtualAllocEx (m_process, begin_mbi.AllocationBase, realloc, MEM_RESERVE, PAGE_NOACCESS)) {
					assert (ERROR_INVALID_ADDRESS == GetLastError ());
					raise_condition ();
				}
			}
		}

		if (end_mbi.BaseAddress) {
			SSIZE_T realloc = (BYTE*)end_mbi.BaseAddress + end_mbi.RegionSize - end;
			if (realloc > 0) {
				if ((BYTE*)end_mbi.AllocationBase >= begin)
					verify (VirtualFreeEx (m_process, end_mbi.AllocationBase, 0, MEM_RELEASE));
				while (!VirtualAllocEx (m_process, end, realloc, MEM_RESERVE, PAGE_NOACCESS)) {
					assert (ERROR_INVALID_ADDRESS == GetLastError ());
					raise_condition ();
				}
			}
		}
	}

	// Release memory
	for (BYTE* p = begin; p < end;) {
		HANDLE mapping = InterlockedExchangePointer (&allocated_block (p)->mapping, 0);
		assert (mapping);
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
				allocated_block (p)->mapping = 0;
				p += ALLOCATION_GRANULARITY;
			}
		} else if (mapping) {
			verify (UnmapViewOfFile2 (m_process, p, 0));
			verify (CloseHandle (mapping));
			p += ALLOCATION_GRANULARITY;
		}
	}
}

void* AddressSpace::map (HANDLE mapping, MappingType protection)
{
	assert (mapping);
	assert (protection == MAP_SHARED || is_current_process ());
	for (;;) {
		void* p = MapViewOfFile2 (mapping, m_process, 0, 0, ALLOCATION_GRANULARITY, 0, protection);
		if (!p)
			throw NO_MEMORY ();
		try {
			if (!InterlockedCompareExchangePointer (&block (p).mapping, mapping, 0))
				return p;
		} catch (...) {
			UnmapViewOfFile2 (m_process, p, 0);
			throw;
		}
		UnmapViewOfFile2 (m_process, p, 0);
		raise_condition ();
	}
}

void* AddressSpace::copy (HANDLE src, SIZE_T offset, SIZE_T size, LONG flags)
{
	HANDLE mapping;
	if (!DuplicateHandle (GetCurrentProcess (), src, m_process, &mapping, 0, FALSE, DUPLICATE_SAME_ACCESS))
		throw NO_MEMORY ();

	BYTE* p;
	try {
		p = (BYTE*)map (mapping, MAP_SHARED);
	} catch (...) {
		CloseHandle (mapping);
		throw;
	}

	if (Memory::READ_ONLY & flags)
		protect (p + offset, size, PageState::RO_MAPPED_SHARED);

	if (offset > 0)
		protect (p, round_down (offset, PAGE_SIZE), PageState::DECOMMITTED);
	offset = round_up (offset + size, PAGE_SIZE);
	SIZE_T tail = ALLOCATION_GRANULARITY - offset;
	if (tail > 0)
		protect (p + offset, tail, PageState::DECOMMITTED);

	return p;
}

void AddressSpace::check_allocated (void* ptr, SIZE_T size)
{
	if (!size)
		return;
	if (!ptr)
		throw BAD_PARAM ();

	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end; p += ALLOCATION_GRANULARITY)
		if (!allocated_block (p))
			throw BAD_PARAM ();
}

DWORD AddressSpace::check_committed (void* ptr, SIZE_T size)
{
	if (!size)
		return 0;
	if (!ptr)
		throw BAD_PARAM ();

	DWORD mask;
	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end;) {
		Block block (*this, p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		mask |= block.check_committed (p - block.address (), block_end - p);
		p = block_end;
	}
	return mask;
}

void AddressSpace::change_protection (void* ptr, SIZE_T size, LONG flags)
{
	if (!size)
		return;
	if (!ptr)
		throw BAD_PARAM ();

	BYTE* begin = (BYTE*)ptr;
	BYTE* end = begin + size;
	for (BYTE* p = begin; p < end;) {
		Block block (*this, p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.change_protection (p - block.address (), block_end - p, flags);
		p = block_end;
	}
}

void AddressSpace::decommit (void* ptr, SIZE_T size)
{
	if (!size)
		return;

	// Memory must be allocated.
	check_allocated (ptr, size);

	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end;) {
		Block block (*this, p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.decommit (p - block.address (), block_end - p);
		p = block_end;
	}
}

}
}
