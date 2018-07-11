// Nirvana project
// Protection domain memory service over Win32 API

#ifndef NIRVANA_MEMORYWINDOWS_H_
#define NIRVANA_MEMORYWINDOWS_H_

#include <Memory.h>
#include "AddressSpace.h"
#include "../real_copy.h"
#include <eh.h>

namespace Nirvana {
namespace Windows {

using namespace ::CORBA;

class MemoryWindows :
	public ::CORBA::Nirvana::ServantStatic <MemoryWindows, ::Nirvana::Memory>
{
public:
	static void initialize ()
	{
		sm_space.initialize ();
		sm_exception_filter = SetUnhandledExceptionFilter (&exception_filter);
		_set_se_translator (&se_translator);
	}

	static void terminate ()
	{
		SetUnhandledExceptionFilter (sm_exception_filter);
		sm_space.terminate ();
	}

	// Memory::
	static void* allocate (void* dst, SIZE_T size, LONG flags)
	{
		if (!size)
			throw BAD_PARAM ();

		if (flags & ~(Memory::RESERVED | Memory::EXACTLY | Memory::ZERO_INIT))
			throw INV_FLAG ();

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

	static bool is_readable (const void* p, SIZE_T size)
	{
		return sm_space.is_readable (p, size);
  }

	static bool is_writable (const void* p, SIZE_T size)
	{
		return sm_space.is_writable (p, size);
	}

	static bool is_private (const void* p, SIZE_T size)
	{
		return sm_space.is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, SIZE_T size)
	{
		return sm_space.is_copy (p1, p2, size);
	}

	static SIZE_T query (const void* p, Memory::QueryParam q);

	static void prepare_to_share (void* src, SIZE_T size);

	struct Region
	{
		void* ptr;
		SIZE_T size;

		SIZE_T subtract (void* begin, void* end)
		{
			BYTE* my_end = (BYTE*)ptr + size;
			if (ptr < begin) {
				if (my_end >= end)
					size = 0;
				if (my_end > begin)
					size -= my_end - (BYTE*)begin;
			} else if (end >= my_end)
				size = 0;
			else if (end > ptr) {
				size -= (BYTE*)end - (BYTE*)ptr;
				ptr = end;
			}
			return size;
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
		static void remap_proc (Block* block);
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
		//sm_space.protect (address, size, protection);
		DWORD old;
		verify (VirtualProtect (address, size, protection, &old));
	}

	static void query (const void* address, MEMORY_BASIC_INFORMATION& mbi)
	{
		//sm_space.query (address, mbi);
		verify (VirtualQuery (address, &mbi, sizeof (mbi)));
	}

	// Create new mapping
	static HANDLE new_mapping ()
	{
		HANDLE mapping = CreateFileMappingW (0, 0, PAGE_EXECUTE_READWRITE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0);
		if (!mapping)
			throw NO_MEMORY ();
		return mapping;
	}

	// Thread stack processing

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

	static LONG CALLBACK exception_filter (struct _EXCEPTION_POINTERS* pex);
	static void se_translator (unsigned int, struct _EXCEPTION_POINTERS* pex);

private:
	static AddressSpace sm_space;
	static PTOP_LEVEL_EXCEPTION_FILTER sm_exception_filter;
};

inline void* MemoryWindows::copy (void* dst, void* src, SIZE_T size, LONG flags)
{
	if (!size)
		return dst;

	// Source range have to be committed.
	DWORD src_prot_mask = sm_space.check_committed (src, size);
	
	void* stack_base, *stack_limit, *stack_top;
	{
		const NT_TIB* tib = current_TIB ();
		stack_base = tib->StackBase;
		stack_limit = tib->StackLimit;
		MEMORY_BASIC_INFORMATION mbi;
		query ((BYTE*)stack_base - PAGE_SIZE, mbi);
		stack_top = mbi.AllocationBase;
	}

	bool src_in_stack = false, dst_in_stack = false;
	if (src >= stack_limit) {
		if (
			(src_in_stack = src < stack_base)
		&&
			(flags & Memory::DECOMMIT)	// Memory::RELEASE also includes flag DECOMMIT.
		)
			throw BAD_PARAM ();
	} else if (stack_top <= src)
		throw BAD_PARAM ();

	void* ret = 0;
	SIZE_T src_align = (SIZE_T)src % ALLOCATION_GRANULARITY;
	try {
		if (!dst && Memory::RELEASE != (flags & Memory::RELEASE) && !src_in_stack && round_up ((BYTE*)src + size, ALLOCATION_GRANULARITY) - (BYTE*)src <= ALLOCATION_GRANULARITY) {
			// Quick copy one block.
			Block block (src);
			block.prepare_to_share (src_align, size);
			ret = sm_space.copy (block.mapping (), src_align, size, flags);
		} else {
			Region allocated = {0, 0};
			if (!dst || (flags & Memory::ALLOCATE)) {
				if (dst) {
					if (dst == src) {
						if ((Memory::EXACTLY & flags) && Memory::RELEASE != (flags & Memory::RELEASE))
							return 0;
					} else {
						// Try reserve space exactly at dst.
						// Target region can overlap with source.
						allocated.ptr = dst;
						allocated.size = size;
						if (
							allocated.subtract (round_down (src, ALLOCATION_GRANULARITY), round_up ((BYTE*)src + size, ALLOCATION_GRANULARITY))
						&&
							sm_space.reserve (allocated.size, flags | Memory::EXACTLY, allocated.ptr)
						)
							ret = dst;
						else if (flags & Memory::EXACTLY)
							return 0;
					}
				}
				if (!ret) {
					if (Memory::RELEASE == (flags & Memory::RELEASE))
						ret = src;
					else {
						BYTE* res = (BYTE*)sm_space.reserve (size + src_align, flags);
						if (!res)
							return 0;
						ret = res + src_align;
						allocated.ptr = ret;
						allocated.size = size;
					}
				}
			} else {
				if (dst >= stack_limit)
					dst_in_stack = dst < stack_base;
				else if (stack_top <= dst)
					throw BAD_PARAM ();
				sm_space.check_allocated (dst, size);
				ret = dst;
			}

			assert (ret);

			if (ret == src) { // Special case - change protection.
				if ((Memory::ALLOCATE & flags) &&  Memory::RELEASE != (flags & Memory::RELEASE)) {
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

			try {
				if (!src_in_stack && !dst_in_stack && (SIZE_T)ret % ALLOCATION_GRANULARITY == src_align) {
					// Share (regions may overlap).
					if (ret < src) {
						BYTE* pd = (BYTE*)ret, *end = pd + size;
						BYTE* ps = (BYTE*)src;
						if (end > src) {
							// Copy overlapped part with Memory::DECOMMIT.
							BYTE* first_part_end = round_up (end - ((BYTE*)src + size - end), ALLOCATION_GRANULARITY);
							assert (first_part_end < end);
							LONG first_part_flags = (flags & ~Memory::RELEASE) | Memory::DECOMMIT;
							while (pd < first_part_end) {
								Block block (pd);
								BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
								SIZE_T cb = block_end - pd;
								block.copy (ps, cb, first_part_flags);
								pd = block_end;
								ps += cb;
							}
						}
						while (pd < end) {
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
						BYTE* src_end = (BYTE*)src + size;
						BYTE* pd = (BYTE*)ret + size, *ps = (BYTE*)src + size;
						if (ret < src_end) {
							// Copy overlapped part with Memory::DECOMMIT.
							BYTE* first_part_begin = round_down ((BYTE*)ret + ((BYTE*)ret - (BYTE*)src), ALLOCATION_GRANULARITY);
							assert (first_part_begin > ret);
							LONG first_part_flags = (flags & ~Memory::RELEASE) | Memory::DECOMMIT;
							while (pd > first_part_begin) {
								BYTE* block_begin = round_down (pd - 1, ALLOCATION_GRANULARITY);
								Block block (block_begin);
								SIZE_T cb = pd - block_begin;
								ps -= cb;
								block.copy (ps, cb, first_part_flags);
								pd = block_begin;
							}
						}
						while (pd > ret) {
							BYTE* block_begin = round_down (pd - 1, ALLOCATION_GRANULARITY);
							if (block_begin < ret)
								block_begin = (BYTE*)ret;
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

					if ((flags & Memory::DECOMMIT) && ret != src) {
						// Release or decommit source. Regions can overlap.
						Region reg = {src, size};
						if (flags & (Memory::RELEASE & ~Memory::DECOMMIT)) {
							if (reg.subtract (round_up (ret, ALLOCATION_GRANULARITY), round_down ((BYTE*)ret + size, ALLOCATION_GRANULARITY)))
								release (reg.ptr, reg.size);
						} else {
							if (reg.subtract (round_up (ret, PAGE_SIZE), round_down ((BYTE*)ret + size, PAGE_SIZE)))
								decommit (reg.ptr, reg.size);
						}
					}
				}
			} catch (...) {
				release (allocated.ptr, allocated.size);
				throw;
			}
		}
	} catch (const NO_MEMORY&) {
		if (Memory::EXACTLY & flags)
			ret = 0;
		else
			throw;
	}

	return ret;
}

inline SIZE_T MemoryWindows::query (const void* p, Memory::QueryParam q)
{
	{
		switch (q) {

		case Memory::ALLOCATION_SPACE_BEGIN:
			{
				SYSTEM_INFO sysinfo;
				GetSystemInfo (&sysinfo);
				return (SIZE_T)sysinfo.lpMinimumApplicationAddress;
			}

		case Memory::ALLOCATION_SPACE_END:
			return (SIZE_T)sm_space.end ();

		case Memory::ALLOCATION_UNIT:
		case Memory::SHARING_UNIT:
		case Memory::GRANULARITY:
		case Memory::SHARING_ASSOCIATIVITY:
		case Memory::OPTIMAL_COMMIT_UNIT:
			return ALLOCATION_GRANULARITY;

		case Memory::PROTECTION_UNIT:
		case Memory::COMMIT_UNIT:
			return PAGE_SIZE;

		case Memory::FLAGS:
			return
				Memory::ACCESS_CHECK |
				Memory::HARDWARE_PROTECTION |
				Memory::COPY_ON_WRITE |
				Memory::SPACE_RESERVATION;
		}

		throw BAD_PARAM ();
	}
}

}
}

#endif  // _WIN_MEMORY_H_
