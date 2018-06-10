// Nirvana project
// Protection domain memory service over Win32 API

#include "MemoryWindows.h"
#include <cpplib.h>
#include "../real_copy.h"

namespace Nirvana {
namespace Windows {

AddressSpace MemoryWindows::sm_space;

inline void MemoryWindows::Block::commit (void* ptr, SIZE_T size)
{
	BYTE* begin = round_up ((BYTE*)ptr, PAGE_SIZE);
	BYTE* end = round_down ((BYTE*)ptr + size, PAGE_SIZE);
	assert (begin >= address ());
	assert (end <= address () + ALLOCATION_GRANULARITY);

	HANDLE old_mapping = mapping ();
	if (INVALID_HANDLE_VALUE == old_mapping) {
		HANDLE hm = new_mapping ();
		try {
			map (hm, AddressSpace::MAP_PRIVATE);
			if (!VirtualAlloc (begin, size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE)) {
				unmap ();
				throw NO_MEMORY ();
			}
		} catch (...) {
			CloseHandle (hm);
			throw;
		}
	} else {
		const State& bs = state ();
		Regions regions;
		auto region_begin = bs.mapped.page_state, block_end = region_begin + PAGES_PER_BLOCK;
		do {
			auto region_end = region_begin;
			DWORD state = *region_begin;
			do
				++region_end;
			while (region_end < block_end && state == *region_end);

			if (!(PageState::MASK_ACCESS & state))
				regions.add (address () + (region_begin - bs.mapped.page_state) * PAGE_SIZE, (region_end - region_begin) * PAGE_SIZE);

			region_begin = region_end;
		} while (region_begin < block_end);

		if (regions.end != regions.begin) {
			if (bs.page_state_bits & PageState::MASK_MAY_BE_SHARED)
				remap (false);
			for (const Region* p = regions.begin; p != regions.end; ++p)
				verify (VirtualAlloc (p->ptr, p->size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE));
		}
	}
}

inline void MemoryWindows::Block::decommit (void* ptr, SIZE_T size)
{
	BYTE* begin = round_up((BYTE*)ptr, PAGE_SIZE);
	BYTE* end = round_down ((BYTE*)ptr + size, PAGE_SIZE);
	assert (begin >= address ());
	assert (end <= address () + ALLOCATION_GRANULARITY);
	if (begin == address () && end == address () + ALLOCATION_GRANULARITY)
		unmap ();
	else if (state ().state == State::MAPPED) {
		bool can_unmap = true;
		auto page_state = state ().mapped.page_state;
		if (begin > address ()) {
			for (auto ps = page_state, end = page_state + (begin - address ()) / PAGE_SIZE; ps < end; ++ps) {
				if (PageState::MASK_ACCESS & *ps) {
					can_unmap = false;
					break;
				}
			}
		}
		if (can_unmap && end < address () + ALLOCATION_GRANULARITY) {
			for (auto ps = page_state + (end - address ()) / PAGE_SIZE, end = page_state + PAGES_PER_BLOCK; ps < end; ++ps) {
				if (PageState::MASK_ACCESS & *ps) {
					can_unmap = false;
					break;
				}
			}
		}

		if (can_unmap)
			unmap ();
		else {
			invalidate_state ();
			// Decommit pages. We can't use VirtualFree and MEM_DECOMMIT with mapped memory.
			protect (begin, size, PageState::DECOMMITTED | PAGE_REVERT_TO_FILE_MAP);
			// Discard page. TODO: Is this safe if page is shared?
			verify (VirtualAlloc (begin, size, MEM_RESET, PageState::DECOMMITTED));
		}
	}
}

void MemoryWindows::Block::prepare2share (void* begin, SIZE_T size)
{
	SIZE_T offset = (BYTE*)begin - address ();
	assert (offset + size <= ALLOCATION_GRANULARITY);

	if (state ().state != State::MAPPED)
		throw BAD_PARAM ();

	// Remap if neeed.
	bool need_remap = false;
	auto page_state = state ().mapped.page_state;
	for (auto ps = page_state + offset / PAGE_SIZE, end = page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps) {
		DWORD state = *ps;
		if (!(PageState::MASK_ACCESS & state))
			throw BAD_PARAM ();	// Page not committed
		else if (PageState::MASK_UNMAPPED & state)
			need_remap = true;
	}
	if (state ().page_state_bits & PageState::DECOMMITTED)
		need_remap = true;

	if (need_remap)
		remap (true);
	else { // Prepare pages
		auto page_state = state ().mapped.page_state;
		auto region_begin = page_state, block_end = region_begin + PAGES_PER_BLOCK;
		bool committed = false;
		do {
			auto region_end = region_begin;
			DWORD state = *region_begin;
			do
				++region_end;
			while (region_end < block_end && state == *region_end);

			if (PageState::RW_MAPPED_PRIVATE == state) {
				BYTE* ptr = address () + (region_begin - page_state) * PAGE_SIZE;
				SIZE_T size = (region_end - region_begin) * PAGE_SIZE;
				protect (ptr, size, PageState::RW_MAPPED_SHARED);
			}

			region_begin = region_end;
		} while (region_begin < block_end);
	}
}

void MemoryWindows::Block::remap (bool for_share)
{
	HANDLE hm = new_mapping ();
	try {
		BYTE* ptmp = (BYTE*)MapViewOfFile (hm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (!ptmp)
			throw NO_MEMORY ();

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
					if (!VirtualAlloc (dst, size, MEM_COMMIT, PAGE_READWRITE))
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
			UnmapViewOfFile (ptmp);
			throw;
		}
		verify (UnmapViewOfFile (ptmp));

		// Change to new mapping
		AddressSpace::MappingType mt;
		DWORD read_only_protect;
		if (for_share) {
			mt = AddressSpace::MAP_SHARED;
			read_only_protect = PageState::RO_MAPPED_SHARED;
		} else {
			mt = AddressSpace::MAP_PRIVATE;
			read_only_protect = PageState::RO_MAPPED_PRIVATE;
		}
		map (hm, mt);

		// Change protection for read-only pages.
		for (const Region* p = read_only.begin; p != read_only.end; ++p)
			protect (p->ptr, p->size, read_only_protect);
	} catch (...) {
		CloseHandle (hm);
		throw;
	}
}

void MemoryWindows::commit (void* dst, SIZE_T size)
{
	if (!size)
		return;
	if (!dst)
		throw BAD_PARAM ();

	BYTE* begin = (BYTE*)dst;
	BYTE* end = begin + size;

	// Memory must be allocated.
	for (BYTE* p = begin; p < end; p += ALLOCATION_GRANULARITY)
		sm_space.allocated_block (p);

	BYTE* p = begin;
	try {
		while (p < end) {
			Block block (p);
			BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
			if (block_end > end)
				block_end = end;
			block.commit (p, block_end - p);
			p = block_end;
		}
	} catch (...) {
		decommit (begin, p - begin);
		throw;
	}
}

void MemoryWindows::decommit (void* dst, SIZE_T size)
{
	if (!size)
		return;
	if (!dst)
		throw BAD_PARAM ();

	BYTE* begin = (BYTE*)dst;
	BYTE* end = begin + size;

	// Memory must be allocated.
	for (BYTE* p = begin; p < end; p += ALLOCATION_GRANULARITY)
		sm_space.allocated_block (p);

	for (BYTE* p = begin; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.decommit (p, block_end - p);
		p = block_end;
	}
}

NT_TIB* MemoryWindows::current_tib ()
{
#ifdef _M_IX86
	return (NT_TIB*)__readfsdword (0x18);
#elif _M_AMD64
	return (NT_TIB*)__readgsqword (0x30);
#endif
}

void MemoryWindows::thread_prepare ()
{
	// Prepare stack of current thread to share

	// Obtain stack size
	StackPrepareParam param;
	const NT_TIB* ptib = current_tib ();
	param.stack_base = ptib->StackBase;
	param.stack_limit = ptib->StackLimit;

	// Call share_stack in fiber
	verify (ConvertThreadToFiber (0));
//	call_in_fiber ((FiberMethod)&MemoryWindows::stack_prepare, &param);
}

void MemoryWindows::thread_unprepare ()
{}
/*
void MemoryWindows::stack_prepare (const StackPrepareParam* param_ptr)
{
	// Copy parameter from source stack
	// The source stack will become temporary unaccessible
	ShareStackParam param = *param_ptr;

	// Temporary copy committed stack
	UWord cur_stack_size = (Octet*)param.stack_base - (Octet*)param.stack_limit;
	void* ptmp = VirtualAlloc (0, cur_stack_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!ptmp)
		throw NO_MEMORY ();

	//memcpy (ptmp, param.stack_limit, cur_stack_size);
	real_copy ((const UWord*)param.stack_limit, (const UWord*)param.stack_base, (UWord*)ptmp);

	// Get stack region
	// The first page before param.m_stack_limit is a guard page
	MemoryBasicInformation mbi ((Page*)param.stack_limit - 1);

	assert (MEM_COMMIT == mbi.State);
	assert ((PAGE_GUARD | PAGE_READWRITE) == mbi.Protect);
	assert (PAGE_SIZE == mbi.RegionSize);

	// Decommit Windows memory
	VirtualFree (mbi.BaseAddress, (Octet*)param.stack_base - (Octet*)mbi.BaseAddress, MEM_DECOMMIT);

#ifdef _DEBUG
	{
		MemoryBasicInformation dmbi (mbi.AllocationBase);

		assert (dmbi.State == MEM_RESERVE);
		assert (mbi.AllocationBase == dmbi.AllocationBase);
		assert ((((Octet*)mbi.AllocationBase) + dmbi.RegionSize) == (Octet*)param.stack_base);
	}
#endif // _DEBUG

	Block* stack_begin = mbi.allocation_base ();
	assert (!((UWord)param.stack_base % ALLOCATION_GRANULARITY));
	Block* stack_end = (Block*)param.stack_base;
	Block* line = stack_begin;

	try {

		// Map our memory (but not commit)
		do
			map (line);
		while (++line < stack_end);

	} catch (...) {

		// Unmap
		while (line > stack_begin)
			unmap (--line);

		// Commit current stack
		VirtualAlloc (param.stack_limit, cur_stack_size, MEM_COMMIT, PAGE_READWRITE);

		// Commit guard page(s)
		VirtualAlloc (mbi.BaseAddress, mbi.RegionSize, MEM_COMMIT, PAGE_GUARD | PAGE_READWRITE);

		// Release temporary buffer
		VirtualFree (ptmp, 0, MEM_RELEASE);

		throw;
	}

	// Commit current stack
	VirtualAlloc (param.stack_limit, cur_stack_size, MEM_COMMIT, PAGE_READWRITE);

	// Commit guard page(s)
	VirtualAlloc ((Page*)param.stack_limit - 1, mbi.RegionSize, MEM_COMMIT, PAGE_GUARD | PAGE_READWRITE);

	// Copy stack data back
	//memcpy (param.stack_limit, ptmp, cur_stack_size);
	real_copy ((const UWord*)ptmp, (const UWord*)((Octet*)ptmp + cur_stack_size), (UWord*)param.stack_limit);

	// Release temporary buffer
	VirtualFree (ptmp, 0, MEM_RELEASE);

	assert (!memcmp (&param, param_ptr, sizeof (ShareStackParam)));
}
*/
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
