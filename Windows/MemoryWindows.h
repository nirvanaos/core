// Nirvana project.
// Protection domain memory service for Win32 API

#ifndef NIRVANA_MEMORYWINDOWS_H_
#define NIRVANA_MEMORYWINDOWS_H_

#include <Memory.h>
#include "AddressSpace.h"

namespace Nirvana {
namespace Windows {

using namespace ::CORBA;
using namespace ::CORBA::Nirvana;

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
		if (!dst && size <= ALLOCATION_GRANULARITY && !(Memory::RESERVED & flags)) {
			// Optimization: quick allocate
			
			HANDLE mapping = new_mapping ();
			try {
				ret = sm_space.map (mapping, protection);
			} catch (...) {
				CloseHandle (mapping);
				throw;
			}

			if (!(Memory::RESERVED & flags))
				verify (VirtualAlloc (ret, size, MEM_COMMIT, protection));

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

		return ret;
	}

	static void release (void* dst, SIZE_T size)
	{
		sm_space.release (dst, size);
	}

	static void commit (void* dst, SIZE_T size);
	static void decommit (void* dst, SIZE_T size);

	static void* copy (void* dst, void* src, SIZE_T size, LONG flags);

	static Boolean is_private (Pointer p);
	static Boolean is_copy (Pointer p1, Pointer p2, UWord size);
	static Word query (Pointer p, Memory::QueryParam q);

	class Block :
		public AddressSpace::Block
	{
	public:
		Block (void* addr) :
			AddressSpace::Block (sm_space, addr)
		{}

		void commit (void* begin, SIZE_T size);
		void decommit (void* begin, SIZE_T size);
		void prepare2share (void* begin, SIZE_T size);

	private:
		void decommit_surrogate (void* begin, SIZE_T size);
	};

private:
	static void protect (void* address, SIZE_T size, DWORD protection)
	{
		sm_space.protect (address, size, protection);
	}

	// Create new mapping
	static HANDLE new_mapping ()
	{
		HANDLE mapping = CreateFileMapping (0, 0, PageState::RW_MAPPED_PRIVATE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0);
		if (!mapping)
			throw NO_MEMORY ();
		return mapping;
	}

	// Thread stack processing

	static NT_TIB* current_tib ();

	static void thread_prepare ();
	static void thread_unprepare ();

	typedef void (*FiberMethod) (void*);

	static void call_in_fiber (FiberMethod method, void* param);

	struct StackPrepareParam
	{
		void* stack_base;
		void* stack_limit;
	};

	static void stack_prepare (const StackPrepareParam* param);
	static void stack_unprepare (const StackPrepareParam* param);

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

}
}

#endif  // _WIN_MEMORY_H_
