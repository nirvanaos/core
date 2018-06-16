// Nirvana project
// Protection domain memory service over Win32 API

#include "MemoryWindows.h"
#include <cpplib.h>

namespace Nirvana {
namespace Windows {

AddressSpace MemoryWindows::sm_space;

DWORD MemoryWindows::Block::commit (SIZE_T offset, SIZE_T size)
{
	assert (offset + size <= ALLOCATION_GRANULARITY);

	DWORD ret = 0;	// Page state bits in committed region
	HANDLE old_mapping = mapping ();
	if (INVALID_HANDLE_VALUE == old_mapping) {
		HANDLE hm = new_mapping ();
		try {
			map (hm, AddressSpace::MAP_PRIVATE);
			if (size) {
				if (!VirtualAlloc (address () + offset, size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE)) {
					unmap ();
					throw NO_MEMORY ();
				}
				ret = PageState::RW_MAPPED_PRIVATE;
			}
		} catch (...) {
			CloseHandle (hm);
			throw;
		}
	} else if (size) {
		const State& bs = state ();
		Regions regions;	// Regions to commit.
		auto region_begin = bs.mapped.page_state + offset / PAGE_SIZE, state_end = bs.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
		do {
			auto region_end = region_begin;
			DWORD state = *region_begin;
			if (!(PageState::MASK_ACCESS & state)) {
				do {
					++region_end;
				} while (region_end < state_end && !(PageState::MASK_ACCESS & (state = *region_end)));

				regions.add (address () + (region_begin - bs.mapped.page_state) * PAGE_SIZE, (region_end - region_begin) * PAGE_SIZE);
			} else {
				do {
					ret |= state;
					++region_end;
				} while (region_end < state_end && (PageState::MASK_ACCESS & (state = *region_end)));
			}
			region_begin = region_end;
		} while (region_begin < state_end);

		if (regions.end != regions.begin) {
			if (bs.page_state_bits & PageState::MASK_MAY_BE_SHARED) {
				remap ();
				ret = ((ret & PageState::MASK_RW) ? PageState::RW_MAPPED_PRIVATE : 0)
					| ((ret & PageState::MASK_RO) ? PageState::RO_MAPPED_PRIVATE : 0);
			}
			for (const Region* p = regions.begin; p != regions.end; ++p) {
				if (!VirtualAlloc (p->ptr, p->size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE)) {
					// Error, decommit back and throw the exception.
					while (p != regions.begin) {
						--p;
						protect (p->ptr, p->size, PageState::DECOMMITTED);
						verify (VirtualAlloc (p->ptr, p->size, MEM_RESET, PageState::DECOMMITTED));
					}
					throw NO_MEMORY ();
				}
			}
		}
	}
	return ret;
}

bool MemoryWindows::Block::need_remap_to_share (SIZE_T offset, SIZE_T size)
{
	const State& st = state ();
	if (st.state != State::MAPPED)
		throw BAD_PARAM ();
	if (st.page_state_bits & PageState::MASK_UNMAPPED) {
		if (0 == offset && size == ALLOCATION_GRANULARITY)
			return true;
		else {
			for (auto ps = st.mapped.page_state + offset / PAGE_SIZE, end = st.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps) {
				if (PageState::MASK_UNMAPPED & *ps)
					return true;
			}
		}
	}
	return false;
}

void MemoryWindows::Block::prepare_to_share_no_remap (SIZE_T offset, SIZE_T size)
{
	assert (offset + size <= ALLOCATION_GRANULARITY);
	assert ((state ().state == State::MAPPED));

	// Prepare pages
	const State& st = state ();
	if (st.page_state_bits & PageState::RW_MAPPED_PRIVATE) {
		auto region_begin = st.mapped.page_state + offset / PAGE_SIZE, block_end = st.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
		do {
			auto region_end = region_begin;
			DWORD state = *region_begin;
			do
				++region_end;
			while (region_end < block_end && state == *region_end);

			if (PageState::RW_MAPPED_PRIVATE == state) {
				BYTE* ptr = address () + (region_begin - st.mapped.page_state) * PAGE_SIZE;
				SIZE_T size = (region_end - region_begin) * PAGE_SIZE;
				protect (ptr, size, PageState::RW_MAPPED_SHARED);
			}

			region_begin = region_end;
		} while (region_begin < block_end);
	}
}

void MemoryWindows::Block::remap ()
{
	HANDLE hm = new_mapping ();
	try {
		BYTE* ptmp = (BYTE*)sm_space.map (hm, AddressSpace::MAP_PRIVATE);
		if (!ptmp)
			throw NO_MEMORY ();
		assert (hm == sm_space.allocated_block (ptmp)->mapping);

		Regions read_only;
		try {
			auto page_state = state ().mapped.page_state;
			auto region_begin = page_state, block_end = page_state + PAGES_PER_BLOCK;
			do {
				auto region_end = region_begin;
				if (PageState::MASK_ACCESS & *region_begin) {
					do
						++region_end;
					while (region_end < block_end && (PageState::MASK_ACCESS & *region_end));

					SIZE_T offset = (region_begin - page_state) * PAGE_SIZE;
					LONG_PTR* dst = (LONG_PTR*)(ptmp + offset);
					SIZE_T size = (region_end - region_begin) * PAGE_SIZE;
					if (!VirtualAlloc (dst, size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE))
						throw NO_MEMORY ();
					const LONG_PTR* src = (LONG_PTR*)(address () + offset);
					real_copy (src, src + size / sizeof (LONG_PTR), dst);
					if (PageState::MASK_RO & *region_begin)
						read_only.add ((void*)src, size);
				} else {
					do
						++region_end;
					while (region_end < block_end && !(PageState::MASK_ACCESS & *region_end));
				}

				region_begin = region_end;
			} while (region_begin < block_end);
		} catch (...) {
			sm_space.allocated_block (ptmp)->mapping = 0;
			verify (UnmapViewOfFile (ptmp));
			throw;
		}

		sm_space.allocated_block (ptmp)->mapping = 0;
		verify (UnmapViewOfFile (ptmp));

		// Change to new mapping
		map (hm, AddressSpace::MAP_PRIVATE);

		// Change protection for read-only pages.
		for (const Region* p = read_only.begin; p != read_only.end; ++p)
			protect (p->ptr, p->size, PageState::RO_MAPPED_PRIVATE);
	} catch (...) {
		CloseHandle (hm);
		throw;
	}
}

void MemoryWindows::Block::copy (void* src, SIZE_T size, LONG flags)
{
	assert (size);
	SIZE_T offset = (SIZE_T)src % ALLOCATION_GRANULARITY;
	assert (offset + size <= ALLOCATION_GRANULARITY);

	Block src_block (src);
	if ((offset || size < ALLOCATION_GRANULARITY) && INVALID_HANDLE_VALUE != mapping()) {
		if (CompareObjectHandles (mapping (), src_block.mapping ())) {
			if (src_block.need_remap_to_share (offset, size)) {
				if (has_data_outside_of (offset, size, PageState::MASK_UNMAPPED)) {
					// Real copy
					copy (offset, size, src, flags);
					return;
				} else
					src_block.remap ();
			} else {
				SIZE_T page_off = offset % PAGE_SIZE;
				if (page_off) {
					SIZE_T page_tail = PAGE_SIZE - page_off;
					if (copy_page_part (src, page_tail, flags)) {
						offset += page_tail;
						size -= page_tail;
						assert (!(offset % PAGE_SIZE));
					}
				}
				page_off = (offset + size) % PAGE_SIZE;
				if (page_off && copy_page_part ((const BYTE*)src + size - page_off, page_off, flags)) {
					size -= page_off;
					assert (!((offset + size) % PAGE_SIZE));
				}
				if (!size)
					return;
			}
		} else if (has_data_outside_of (offset, size)) {
			// Real copy
			copy (offset, size, src, flags);
			return;
		} else if (src_block.need_remap_to_share (offset, size))
			src_block.remap ();
	} else if (src_block.need_remap_to_share (offset, size))
		src_block.remap ();

	src_block.prepare_to_share_no_remap (offset, size);
	AddressSpace::Block::copy (src_block.mapping (), offset, size, flags);
}

void MemoryWindows::Block::copy (SIZE_T offset, SIZE_T size, const void* src, LONG flags)
{
	if (PageState::MASK_RO & commit (offset, size))
		change_protection (offset, size, Memory::READ_WRITE);
	real_copy ((const BYTE*)src, (const BYTE*)src + size, address () + offset);
	if (flags & Memory::READ_ONLY)
		change_protection (offset, size, Memory::READ_ONLY);
}

bool MemoryWindows::Block::copy_page_part (const void* src, SIZE_T size, LONG flags)
{
	SIZE_T offset = (SIZE_T)src % ALLOCATION_GRANULARITY;
	DWORD s = state ().mapped.page_state [offset / PAGE_SIZE];
	if (PageState::MASK_UNMAPPED & s) {
		BYTE* dst = address () + offset;
		if (PageState::MASK_RO & s)
			protect (dst, size, PageState::RW_UNMAPPED);
		real_copy ((const BYTE*)src, (const BYTE*)src + size, dst);
		if ((PageState::MASK_RO & s) && (flags & Memory::READ_ONLY))
			protect (dst, size, PageState::RO_UNMAPPED);
		return true;
	}
	return false;
}

DWORD MemoryWindows::commit (void* ptr, SIZE_T size)
{
	if (!size)
		return 0;
	if (!ptr)
		throw BAD_PARAM ();

	// Memory must be allocated.
	sm_space.check_allocated (ptr, size);

	DWORD mask = 0;
	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		mask |= block.commit (p - block.address (), block_end - p);
		p = block_end;
	}
	return mask;
}

void MemoryWindows::prepare_to_share (void* src, SIZE_T size)
{
	if (!size)
		return;
	if (!src)
		throw BAD_PARAM ();

	for (BYTE* p = (BYTE*)src, *end = p + size; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.prepare_to_share (p - block.address (), block_end - p);
		p = block_end;
	}
}

inline MemoryWindows::StackInfo::StackInfo ()
{
	// Obtain stack size.
	const NT_TIB* ptib = current_tib ();
	m_stack_base = (BYTE*)ptib->StackBase;
	m_stack_limit = (BYTE*)ptib->StackLimit;
	assert (m_stack_limit < m_stack_base);
	assert (!((SIZE_T)m_stack_base % PAGE_SIZE));
	assert (!((SIZE_T)m_stack_limit % PAGE_SIZE));

	MEMORY_BASIC_INFORMATION mbi;
#ifdef _DEBUG
	verify (VirtualQuery (m_stack_limit, &mbi, sizeof (mbi)));
	assert (MEM_COMMIT == mbi.State);
	assert (PAGE_READWRITE == mbi.Protect);
#endif

	// The pages before m_stack_limit are guard pages.
	BYTE* guard = m_stack_limit;
	for (;;) {
		BYTE* g = guard - PAGE_SIZE;
		verify (VirtualQuery (g, &mbi, sizeof (mbi)));
		if ((PAGE_GUARD | PAGE_READWRITE) != mbi.Protect)
			break;
		guard = g;
	}
	assert (guard < m_stack_limit);
	m_guard_begin = guard;
	m_allocation_base = (BYTE*)mbi.AllocationBase;
}

MemoryWindows::ThreadMemory::ThreadMemory () :
	StackInfo ()
{ // Prepare stack of current thread to share.
	// Call stack_prepare in fiber
	if (!ConvertThreadToFiber (0))
		throw INTERNAL ();
	call_in_fiber ((FiberMethod)&stack_prepare, this);
}

MemoryWindows::ThreadMemory::~ThreadMemory ()
{
	call_in_fiber ((FiberMethod)&stack_unprepare, this);
}

class MemoryWindows::ThreadMemory::StackMemory :
	protected StackInfo
{
public:
	StackMemory (const StackInfo& thread) :
		StackInfo (thread)
	{
		SIZE_T cur_stack_size = m_stack_base - m_stack_limit;

		m_tmpbuf = (BYTE*)sm_space.reserve (cur_stack_size);
		if (!m_tmpbuf)
			throw NO_MEMORY ();
		if (!VirtualAlloc (m_tmpbuf, cur_stack_size, MEM_COMMIT, PAGE_READWRITE)) {
			sm_space.release (m_tmpbuf, cur_stack_size);
			throw NO_MEMORY ();
		}

		// Temporary copy current stack.
		real_copy ((const SIZE_T*)m_stack_limit, (const SIZE_T*)(m_stack_base), (SIZE_T*)m_tmpbuf);
	}

	void unprepare ()
	{
		// Remove mappings and free memory. But blocks still marekd as reserved.
		for (BYTE* p = m_allocation_base; p < m_stack_base; p += ALLOCATION_GRANULARITY) {
			HANDLE mapping = InterlockedExchangePointer (&sm_space.allocated_block (p)->mapping, INVALID_HANDLE_VALUE);
			if (mapping != INVALID_HANDLE_VALUE) {
				verify (UnmapViewOfFile (p));
				verify (CloseHandle (mapping));
			} else if (!mapping)
				sm_space.allocated_block (p)->mapping = INVALID_HANDLE_VALUE;
			else
				VirtualFree (p, 0, MEM_RELEASE);
		}

		// Reserve all stack.
		while (!VirtualAlloc (m_allocation_base, m_stack_base - m_allocation_base, MEM_RESERVE, PAGE_READWRITE)) {
			assert (ERROR_INVALID_ADDRESS == GetLastError ());
			AddressSpace::raise_condition ();
		}

		// Mark blocks as free
		for (BYTE* p = m_allocation_base; p < m_stack_base; p += ALLOCATION_GRANULARITY)
			sm_space.allocated_block (p)->mapping = 0;

		finalize ();
	}

	~StackMemory ()
	{
		// Release temporary buffer
		SIZE_T cur_stack_size = m_stack_base - m_stack_limit;
		if (m_tmpbuf) {
			verify (VirtualFree (m_tmpbuf, cur_stack_size, MEM_DECOMMIT));
			sm_space.release (m_tmpbuf, cur_stack_size);
		}
	}

protected:
	void finalize ();

private:
	BYTE* m_tmpbuf;
};

void MemoryWindows::ThreadMemory::StackMemory::finalize ()
{
	SIZE_T cur_stack_size = m_stack_base - m_stack_limit;

	// Commit current stack
	if (!VirtualAlloc (m_stack_limit, cur_stack_size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE))
		throw NO_MEMORY ();

	// Commit guard page(s)
	if (!VirtualAlloc (m_guard_begin, m_stack_limit - m_guard_begin, MEM_COMMIT, PAGE_GUARD | PageState::RW_MAPPED_PRIVATE))
		throw NO_MEMORY ();

	// Copy current stack contents back.
	real_copy ((SIZE_T*)m_tmpbuf, (SIZE_T*)m_tmpbuf + cur_stack_size / sizeof (SIZE_T), (SIZE_T*)m_stack_limit);
}

class MemoryWindows::ThreadMemory::StackPrepare :
	private StackMemory
{
public:
	StackPrepare (const ThreadMemory& thread) :
		StackMemory (thread),
		m_finalized (true)
	{
		m_mapped_end = m_allocation_base;
	}

	~StackPrepare ()
	{
		if (!m_finalized) {	// On error.
			// Unmap and free mapped blocks.
			BYTE* ptail = m_mapped_end + ALLOCATION_GRANULARITY;
			if (ptail < m_stack_base)
				VirtualFree (ptail, 0, MEM_RELEASE); // May fail. No verify.
			VirtualFree (m_mapped_end, 0, MEM_RELEASE); // May fail. No verify.
			while (m_mapped_end > m_allocation_base) {
				Block (m_mapped_end -= ALLOCATION_GRANULARITY).unmap ();
				verify (VirtualFree (m_mapped_end, 0, MEM_RELEASE));
			}

			// Reserve Windows stack back.
			verify (VirtualAlloc (m_allocation_base, m_stack_base - m_allocation_base, MEM_RESERVE, PAGE_READWRITE));

			try {
				finalize ();
			} catch (...) {
			}
		}
	}

	void prepare ()
	{
		m_finalized = false;

		// Mark memory as reserved.
		for (BYTE* p = m_allocation_base; p < m_stack_base; p += ALLOCATION_GRANULARITY)
			sm_space.block (p).mapping = INVALID_HANDLE_VALUE;

		// Decommit Windows memory
		verify (VirtualFree (m_guard_begin, m_stack_base - m_guard_begin, MEM_DECOMMIT));

#ifdef _DEBUG
		{
			MEMORY_BASIC_INFORMATION dmbi;
			assert (VirtualQuery (m_allocation_base, &dmbi, sizeof (dmbi)));
			assert (dmbi.State == MEM_RESERVE);
			assert (m_allocation_base == dmbi.AllocationBase);
			assert (m_allocation_base == dmbi.BaseAddress);
			assert (((BYTE*)dmbi.BaseAddress + dmbi.RegionSize) == m_stack_base);
		}
#endif

		// Map all stack, but not commit.
		for (; m_mapped_end < m_stack_base; m_mapped_end += ALLOCATION_GRANULARITY) {
			Block (m_mapped_end).commit (0, 0);
		}

		finalize ();
		m_finalized = true;
	}

private:
	BYTE * m_mapped_end;
	bool m_finalized;
};

void MemoryWindows::ThreadMemory::stack_prepare (const ThreadMemory* param_ptr)
{
	StackPrepare (*param_ptr).prepare ();
}

void MemoryWindows::ThreadMemory::stack_unprepare (const ThreadMemory* param_ptr)
{
	StackMemory (*param_ptr).unprepare ();
}

void MemoryWindows::call_in_fiber (FiberMethod method, void* param)
{
	FiberParam fp;
	fp.source_fiber = GetCurrentFiber ();
	fp.method = method;
	fp.param = param;
	void* fiber = CreateFiber (0, (LPFIBER_START_ROUTINE)fiber_proc, &fp);
	if (!fiber)
		throw INTERNAL ();
	SwitchToFiber (fiber);
	DeleteFiber (fiber);
	fp.environment.check ();
}

void MemoryWindows::fiber_proc (FiberParam* param)
{
	try {
		(*param->method) (param->param);
	} catch (const Exception& ex) {
		param->environment.set_exception (ex);
	} catch (...) {
		param->environment.set_unknown_exception ();
	}
	SwitchToFiber (param->source_fiber);
}

}
}
