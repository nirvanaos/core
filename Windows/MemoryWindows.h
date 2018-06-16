// Nirvana project
// Protection domain memory service over Win32 API

#ifndef NIRVANA_MEMORYWINDOWS_H_
#define NIRVANA_MEMORYWINDOWS_H_

#include <Memory.h>
#include "AddressSpace.h"
#include "../real_copy.h"

namespace Nirvana {
namespace Windows {

using namespace ::CORBA;

class MemoryWindows
{
public:

	static void initialize ()
	{
		sm_space.initialize ();
	}

	static void terminate ()
	{
		sm_space.terminate ();
	}

	// Memory::
	static void* allocate (void* dst, SIZE_T size, LONG flags)
	{
		if (!size)
			throw BAD_PARAM ();

		if ((Memory::READ_ONLY & flags) && !(Memory::RESERVED & flags))
			throw INV_FLAG ();

		DWORD protection = (Memory::READ_ONLY & flags) ? PageState::RO_MAPPED_PRIVATE : PageState::RW_MAPPED_PRIVATE;

		void* ret;
		try {
			if (!dst && size <= ALLOCATION_GRANULARITY && !(Memory::RESERVED & flags)) {
				// Optimization: quick allocate

				HANDLE mapping = new_mapping ();
				try {
					ret = sm_space.map (mapping, AddressSpace::MAP_PRIVATE);
				} catch (...) {
					CloseHandle (mapping);
					throw;
				}

				if (!(Memory::RESERVED & flags))
					if (!VirtualAlloc (ret, size, MEM_COMMIT, protection))
						throw NO_MEMORY ();

			} else {

				if (!(ret = sm_space.reserve (size, flags, dst)))
					return 0;

				if (!(Memory::RESERVED & flags)) {
					try {
						commit (ret, size);
					} catch (...) {
						sm_space.release (ret, size);
						throw;
					}
				}
			}
		} catch (const NO_MEMORY&) {
			if (flags & Memory::EXACTLY)
				ret = 0;
			else
				throw;
		}
		return ret;
	}

	static void release (void* dst, SIZE_T size)
	{
		sm_space.release (dst, size);
	}

	static DWORD commit (void* dst, SIZE_T size);

	static void decommit (void* dst, SIZE_T size)
	{
		sm_space.decommit (dst, size);
	}

	static void* copy (void* dst, void* src, SIZE_T size, LONG flags);
/*
	static Boolean is_private (Pointer p);
	static Boolean is_copy (Pointer p1, Pointer p2, UWord size);
	static Word query (Pointer p, Memory::QueryParam q);
*/

	static void prepare_to_share (void* src, SIZE_T size);

	struct Region
	{
		void* ptr;
		SIZE_T size;

		void subtract (void* p1)
		{
			if (ptr < p1) {
				BYTE* end = (BYTE*)ptr + size;
				if (end > p1)
					size -= end - (BYTE*)p1;
			} else {
				BYTE* end = (BYTE*)p1 + size;
				if (end > ptr) {
					size -= end - (BYTE*)ptr;
					ptr = end;
				}
			}
		}
	};

	class Block :
		public AddressSpace::Block
	{
	public:
		Block (void* addr) :
			AddressSpace::Block (sm_space, addr)
		{}

		DWORD commit (SIZE_T offset, SIZE_T size);
		bool need_remap_to_share (SIZE_T offset, SIZE_T size);
		
		void prepare_to_share (SIZE_T offset, SIZE_T size)
		{
			if (need_remap_to_share (offset, size))
				remap ();
			prepare_to_share_no_remap (offset, size);
		}

		void copy (void* src, SIZE_T size, LONG flags);

	private:
		struct Regions
		{
			Region begin [PAGES_PER_BLOCK];
			Region* end;

			Regions () :
				end (begin)
			{}

			void add (void* ptr, SIZE_T size)
			{
				assert (end < begin + PAGES_PER_BLOCK);
				Region* p = end++;
				p->ptr = ptr;
				p->size = size;
			}
		};

		void remap ();
		bool copy_page_part (const void* src, SIZE_T size, LONG flags);
		void prepare_to_share_no_remap (SIZE_T offset, SIZE_T size);
		void copy (SIZE_T offset, SIZE_T size, const void* src, LONG flags);
	};

private:
	struct StackInfo
	{
		BYTE* m_stack_base;
		BYTE* m_stack_limit;
		BYTE* m_guard_begin;
		BYTE* m_allocation_base;

		StackInfo ();
	};

public:
	class ThreadMemory :
		protected StackInfo
	{
	public:
		ThreadMemory ();
		~ThreadMemory ();

	private:
		static void stack_prepare (const ThreadMemory* param);
		static void stack_unprepare (const ThreadMemory* param);

		class StackMemory;
		class StackPrepare;
	};

private:
	static void protect (void* address, SIZE_T size, DWORD protection)
	{
		sm_space.protect (address, size, protection);
	}

	// Create new mapping
	static HANDLE new_mapping ()
	{
		HANDLE mapping = CreateFileMappingW (0, 0, PageState::RW_MAPPED_PRIVATE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0);
		if (!mapping)
			throw NO_MEMORY ();
		return mapping;
	}

	// Thread stack processing

	static NT_TIB* current_tib ()
	{
#ifdef _M_IX86
		return (NT_TIB*)__readfsdword (0x18);
#elif _M_AMD64
		return (NT_TIB*)__readgsqword (0x30);
#else
#error Only x86 and x64 platforms supported.
#endif
	}

	typedef void (*FiberMethod) (void*);
	static void call_in_fiber (FiberMethod method, void* param);

	struct FiberParam
	{
		void* source_fiber;
		FiberMethod method;
		void* param;
		Environment environment;
	};

	static void CALLBACK fiber_proc (FiberParam* param);

private:
	static AddressSpace sm_space;
};

inline void* MemoryWindows::copy (void* dst, void* src, SIZE_T size, LONG flags)
{
	if (!size)
		return dst;

	// Source range have to be committed.
	DWORD src_prot_mask = sm_space.check_committed (src, size);
	
	if (dst == src) { // Special case - change protection.
		if (Memory::ALLOCATE == (flags & (Memory::ALLOCATE | Memory::RELEASE))) {
			dst = 0;
			if (flags & Memory::EXACTLY)
				return 0;
		} else {
			// Change protection
			if (src_prot_mask & ((flags & Memory::READ_ONLY) ? PageState::MASK_RW : PageState::MASK_RO))
				sm_space.change_protection (src, size, flags);
			return src;
		}
	}

	BYTE* ret = 0;
	SIZE_T src_align = (SIZE_T)src % ALLOCATION_GRANULARITY;
	try {
		if (!dst && round_up ((BYTE*)src + size, ALLOCATION_GRANULARITY) - (BYTE*)src <= ALLOCATION_GRANULARITY) {
			// Quick copy one block.
			Block block (src);
			block.prepare_to_share (src_align, size);
			return sm_space.copy (block.mapping (), src_align, size, flags);
		}

		Region allocated = {0, 0};
		if (!dst || (flags & Memory::ALLOCATE)) {
			if (dst) {
				// Try reserve space exactly at dst.
				// Target region can overlap with source.
				allocated.ptr = dst;
				allocated.size = size;
				allocated.subtract (src);
				if (sm_space.reserve (allocated.size, flags | Memory::EXACTLY, allocated.ptr))
					ret = (BYTE*)dst;
				else if (flags & Memory::EXACTLY)
					return 0; 
			}
			if (!ret) {
				BYTE* res = (BYTE*)sm_space.reserve (size + src_align, flags);
				if (!res)
					return 0;
				ret = res + src_align;
				allocated.ptr = ret;
				allocated.size = size;
			}
		} else {
			sm_space.check_allocated (dst, size);
			ret = (BYTE*)dst;
		}

		assert (ret);

		try {
			if ((SIZE_T)ret % ALLOCATION_GRANULARITY == (SIZE_T)src % ALLOCATION_GRANULARITY) {
				// Share (regions may overlap).
				if (ret < src) {
					for (BYTE* pd = ret, *end = pd + size, *ps = (BYTE*)src; pd < end;) {
						Block block (pd);
						BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
						if (block_end > end)
							block_end = end;
						SIZE_T cb = block_end - pd;
						block.copy (ps, cb, flags);
						pd = block_end;
						ps += cb;
					}
				} else {
					for (BYTE* pd = ret + size, *ps = (BYTE*)src + size; pd > ret;) {
						BYTE* block_begin = round_down (pd - 1, ALLOCATION_GRANULARITY);
						if (block_begin < ret)
							block_begin = ret;
						Block block (block_begin);
						SIZE_T cb = pd - block_begin;
						ps -= cb;
						block.copy (ps, cb, flags);
						pd = block_begin;
					}
				}
			} else {
				// Physical copy.
				DWORD state_bits = commit (ret, size);
				if (state_bits & PageState::MASK_RO)
					sm_space.change_protection (dst, size, Memory::READ_WRITE);
				real_move ((const BYTE*)src, (const BYTE*)src + size, (BYTE*)ret);
				if (flags & Memory::READ_ONLY)
					sm_space.change_protection (ret, size, Memory::READ_ONLY);
			}
		} catch (...) {
			release (allocated.ptr, allocated.size);
			throw;
		}
	} catch (const NO_MEMORY&) {
		if (Memory::EXACTLY & flags)
			ret = 0;
		else
			throw;
	}

	if (flags & (Memory::RELEASE | Memory::DECOMMIT)) {
		// Regions can overlap.
		Region reg = {src, size};
		reg.subtract (ret);
		if (flags & Memory::RELEASE)
			release (reg.ptr, reg.size);
		else
			decommit (reg.ptr, reg.size);
	}

	return ret;
}

}
}

#endif  // _WIN_MEMORY_H_
