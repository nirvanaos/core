// Nirvana Memory Service for Win32

#ifndef NIRVANA_MEMORYWINDOWS_H_
#define NIRVANA_MEMORYWINDOWS_H_

#include <Memory.h>
#include "ProcessMemory.h"

namespace Nirvana {

using namespace ::CORBA;
using namespace ::CORBA::Nirvana;

class MemoryWindows
{
public:

	static void initialize ()
	{
		sm_directory.initialize ();
	}

	static void terminate ()
	{
		sm_directory.terminate ();
	}

	// Memory::
	static Pointer allocate (Pointer dst, UWord size, Flags flags);
	static Pointer copy (Pointer dst, Pointer src, UWord size, Flags flags);
	static void release (Pointer dst, UWord size);
	static void commit (Pointer dst, UWord size);
	static void decommit (Pointer dst, UWord size);
	Boolean is_private (Pointer p);
	Boolean is_copy (Pointer p1, Pointer p2, UWord size);
	static Word query (Pointer p, Memory::QueryParam q);

private:

	// Некоторые операции (copy, commit) можно выполнить разными способами.
	// Для выбора оптимального пути необходимо понятие стоимости операции, которую требуется
	// минимизировать. При выполнении операции производится физическое копирование некоторого
	// количества байт и выделение некоторого количества физических страниц.
	// За единицу стоимости принимается стоимость копирования одного байта.
	// Стоимость выделения одной физической страницы принимается эквивалентной
	// копированию PAGE_ALLOCATE_COST байт.
	// При выборе значения этой константы надо учитывать, что:
	// 1. При выделении системой физической страницы производится ее обнуление.
	//    Эта операция примерно в 2 раза быстрее физического копирования страницы.
	// 2. Сама по себе физическая память, хоть и является возобновляемым ресурсом, в отличие
	//    от процессорного времени, но тоже чего-то стоит.

	enum
	{
		PAGE_ALLOCATE_COST = PAGE_SIZE
	};

	// Некоторые страницы в строке могут быть скопированы системой при их изменении
	// (page_state & PAGE_COPIED). Такие страницы не отображены и не могут быть разделены.
	// Для их разделения, всю строку нужно переотобразить (remap). Переотображение может быть
	// полным (REMAP_FULL) или частичным (REMAP_PART). Частичное переотображение возможно
	// если всем copied логическим страницам соответствуют свободные виртуальные страницы
	// (page_state & PAGE_VIRTUAL_PRIVATE).
	// В этом случае, мы должны отобразить текущий mapping по временному адресу,
	// скопировать туда все copied страницы, и вызвать UnmapViewOfFile/MapViewOfFile.
	// В результате все copied страницы отобразятся в разделяемую память, а отображенные 
	// страницы останутся в прежнем состоянии.
	// Если хотя бы одной copied странице соответствует занятая виртуальная страница
	// ((page_state & PAGE_COPIED) && !(PAGE_STATE & PAGE_VIRTUAL_PRIVATE)), частичное
	// переотображение невозможно. В этом случае мы можем выполнить полное переотображение.
	// Для этого надо выделить новый mapping и скопировать в него все committed страницы
	// (page_state & PAGE_COMMITTED).
	// Заметим, что, при любом типе переотображения мы должна переотобразить все copied
	// страницы, иначе их содержимое будет потеряно.

	enum RemapType
	{
		REMAP_NONE = 0x00,
		REMAP_PART = 0x01,
		REMAP_FULL = 0x02
	};

	class CostOfOperation
	{
	public:

		CostOfOperation (Word cost = 0)
		{
			m_remap_type = REMAP_NONE;
			m_costs [REMAP_NONE] = cost;
			m_costs [REMAP_PART] = cost;
			m_costs [REMAP_FULL] = cost;
		}

		void remap_type (RemapType remap_type)
		{
			m_remap_type |= remap_type;
		}

		RemapType decide_remap () const;

		Word& operator [] (RemapType remap_type)
		{
			return m_costs [remap_type];
		}

		void operator += (Word inc)
		{
			m_costs [REMAP_NONE] += inc;
			m_costs [REMAP_PART] += inc;
			m_costs [REMAP_FULL] += inc;
		}

	protected:
		Word m_remap_type;
		Word m_costs [REMAP_FULL + 1];
	};

	class MemoryBasicInformation : public MEMORY_BASIC_INFORMATION
	{
	public:

		MemoryBasicInformation (const void* p)
		{
			assert (p);
			verify (VirtualQuery (p, this, sizeof (MEMORY_BASIC_INFORMATION)));
		}

		MemoryBasicInformation ()
		{}

		Page* begin () const
		{
			return (Page*)BaseAddress;
		}

		Page* end () const
		{
			return (Page*)((UWord)BaseAddress + RegionSize);
		}

		Line* allocation_base () const
		{
			return (Line*)AllocationBase;
		}

		bool write_copied () const
		{
			return (WIN_WRITE_COPIED == Protect);
		}

		bool committed () const // Not same as MEM_COMMIT!
		{
			return (MEM_COMMIT == State) && (Protect > PAGE_NOACCESS);
		}

		bool operator ++ ()
		{
			verify (VirtualQuery (end (), this, sizeof (MEMORY_BASIC_INFORMATION)));
		}

		void at (const void* p)
		{
			verify (VirtualQuery (p, this, sizeof (MEMORY_BASIC_INFORMATION)));
		}
	};

	class LineRegions
	{
	public:

		class Region
		{
		public:
			UShort offset, size;
		};

		typedef const Region* Iterator;

		LineRegions ()
		{
			m_end = m_regions;
		}

		void push_back (UWord offset, UWord size);

		Iterator begin () const
		{
			return m_regions;
		}

		Iterator end () const
		{
			return m_end;
		}

		bool not_empty () const
		{
			return m_end > m_regions;
		}

	protected:
		Region  m_regions [10];
		Region* m_end;
	};

	// Line
	typedef ProcessMemory::VirtualLine VirtualLine;

	// Win32 protection types for shared and private memory

	enum
	{
		WIN_NO_ACCESS = PAGE_NOACCESS,

		WIN_WRITE_RESERVE = PAGE_READWRITE,
		WIN_READ_RESERVE = PAGE_READONLY,

		WIN_WRITE_MAPPED_PRIVATE = PAGE_EXECUTE_READWRITE,
		WIN_WRITE_MAPPED_SHARED = PAGE_WRITECOPY,
		WIN_WRITE_COPIED = PAGE_READWRITE,
		WIN_READ_MAPPED = PAGE_EXECUTE_READ,
		WIN_READ_COPIED = PAGE_EXECUTE,

		WIN_MASK_WRITE = PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY,
		WIN_MASK_READ = PAGE_READONLY | PAGE_EXECUTE_READ | PAGE_EXECUTE,
		WIN_MASK_COMMITTED = WIN_MASK_READ | WIN_MASK_WRITE,
		WIN_MASK_MAPPED = WIN_WRITE_MAPPED_PRIVATE | WIN_WRITE_MAPPED_SHARED | WIN_READ_MAPPED,
		WIN_MASK_COPIED = WIN_WRITE_COPIED | WIN_READ_COPIED
	};

	// Page states

	enum
	{
		PAGE_NOT_COMMITTED = 0x00, // Virtual page newer been committed
		PAGE_DECOMMITTED = 0x01,   // Logical page decommitted
		PAGE_MAPPED_SHARED = 0x02, // Logical page mapped to virtual
		PAGE_COPIED = 0x03,        // Logical page write-copied (unmapped)

		PAGE_COMMITTED = 0x02,     // Committed bit

		PAGE_VIRTUAL_PRIVATE = 0x04, // Flag that virtual page not mapped at other lines

		PAGE_MAPPED_PRIVATE = PAGE_MAPPED_SHARED | PAGE_VIRTUAL_PRIVATE
	};

	// Line state information

	struct LineState
	{
		DWORD allocation_protect;
		BYTE page_states [PAGES_PER_LINE];
	};

	static void get_line_state (const Line* line, LineState& state);

	// Logical space reservation

	static Line* reserve (UWord size, UWord flags, Line* begin = 0);

	// Release memory

	static void release (Line* begin, Line* end);

	static UWord check_allocated (Page* begin, Page* end);
	// Returns bitwise or of allocation protect for all lines.

	// Map logical line to virtual

	static bool map (Line* line, HANDLE mapping = 0);
	// line must be allocated.
	// If mapping = 0
	//   if line is mapped,
	//     do nothing
	//   else
	//     allocate new mapping and map
	//     page states will be not committed
	// else
	//   replace current mapping, if exist
	//   Page states are depends from virtual page states.
	// Returns: true if mapping replaced to new.
	// This function is stack-safe. If line is part of current used
	// stack, map_safe will be called through call_in_fiber in separate stack space.

	// Fiber stub for map function
	struct MapParam
	{
		Line* line;
		HANDLE mapping;
		bool ret;
	};

	static void map_in_fiber (MapParam* param);

	static void map (Line* dst, const Line* src);
	// Map source line to destination.
	// Source line must be mapped.
	// Destination page states are not committed.

	// Unmap logical line

	static void unmap (Line* line);
	// If line not mapped, do nothing.
	// Else, line will be decommitted.

	// Unmap and release logical line
	static void unmap_and_release (Line* line)
	{
		unmap (line, virtual_line (line));
	}

	// Assistant function
	static void unmap (Line* line, VirtualLine& vl);
	// Logical line must be mapped.

	// Get current mapping

	static HANDLE mapping (const void* p);  // no throw

	// Create new mapping

	static HANDLE new_mapping ();

	// Remap line

	static HANDLE remap_line (Octet* exclude_begin, Octet* exclude_end, LineState& line_state, Word remap_type);
	// Remaps Line::begin (exclude_begin).
	// Allocated data will be preserved excluding region [exclude_begin, exclude_end).
	// Page state in range [exclude_begin, exclude_end) will be undefined.

	// Committing

	static void commit (Page* begin, Page* end, UWord zero_init);
	// Carefully: does not validates memory state.
	// Changes protection of pages in range [begin, end) to read-write.

	static void commit_line_cost (const Octet* begin, const Octet* end, const LineState& line_state, CostOfOperation& cost, bool copy);
	// Adds cost of commit in range [begin, end) to cost.
	// Cost includes make pages in range [begin, end) private.
	// Если copy = true, функция не учитывает в стоимости переотображения стоимость копирования end - begin байт.
	// Это - режим для подсчета стоимости физического копирования.
	// Для подсчета стоимости фиксации copy = false.

	static void commit_one_line (Page* begin, Page* end, LineState& line_state, Flags zero_init);
	// Changes protection of pages in range [begin, end) to read-write

	static void decommit (Page* begin, Page* end);
	static void decommit_surrogate (Page* begin, UWord size)
	{
		assert (!((UWord)begin % PAGE_SIZE));
		assert (!(size % PAGE_SIZE));
		DWORD old;
		verify (VirtualProtect (begin, size, PAGE_NOACCESS, &old));
		verify (VirtualAlloc (begin, size, MEM_RESET, PAGE_NOACCESS));
	}

	// Copying functions

	static void copy_one_line (Octet* dst, Octet* src, UWord size, UWord flags);
	static bool copy_one_line_aligned (Octet* dst, Octet* src, UWord size, UWord flags);
	static void copy_one_line_really (Octet* dst, const Octet* src, UWord size, RemapType remap_type, LineState& dst_state);

	// Pointer to line number and vice versa conversions

	static void check_pointer (const void* p)
	{
		if (p >= sm_data.address_space_end)
			throw BAD_PARAM ();
	}

	static UWord line_index (const void* p)
	{
		return (UWord)p / LINE_SIZE;
	}

	static Line* line_address (UWord line_index)
	{
		return (Line*)(line_index * LINE_SIZE);
	}

	// Get virtual line information for memory address

	static VirtualLine& virtual_line (const void* p)
	{
		return sm_directory.line (p);
	}

	// Thread stack processing

	static NT_TIB* current_tib ();

	static void thread_prepare ();
	static void thread_unprepare ();

	typedef void (*FiberMethod) (void*);

	static void call_in_fiber (FiberMethod method, void* param);

	struct ShareStackParam
	{
		void* stack_base;
		void* stack_limit;
	};

	static void stack_prepare (const ShareStackParam* param);
	static void stack_unprepare (const ShareStackParam* param);

	struct FiberParam
	{
		void* source_fiber;
		FiberMethod method;
		void* param;
		Environment environment;
	};

	static void CALLBACK fiber_proc (FiberParam* param);

private:
	static ProcessMemory sm_directory;
};

inline Pointer MemoryWindows::allocate (Pointer dst, UWord size, Flags flags)
{
	if (!size)
		throw BAD_PARAM ();

	if ((Memory::READ_ONLY & flags) && !(Memory::RESERVED & flags))
		throw INV_FLAG ();

	// Allocation unit is one line (64K)
	Line* begin = Line::begin (dst);
	UWord offset = (Octet*)dst - begin->pages->bytes;
	if (offset && !(flags & Memory::EXACTLY)) {
		begin = 0;
		offset = 0;
	}

	if (begin) {

		if (!reserve (size + offset, flags, begin)) {

			if (flags & Memory::EXACTLY)
				return 0;

			begin = reserve (size, flags);
		}

	} else {

		if (flags & Memory::EXACTLY)
			throw BAD_PARAM ();

		begin = reserve (size, flags);
	}

	dst = begin->pages->bytes + offset;

	if (!(flags & Memory::RESERVED)) {

		Octet* end = (Octet*)dst + size;

		try {
			commit (Page::begin (dst), Page::end (end), flags & Memory::ZERO_INIT);
		} catch (...) {
			release (begin, Line::end (end));
			throw;
		}
	}

	return dst;
}

inline Pointer MemoryWindows::copy (Pointer dst, Pointer src, UWord size, Flags flags)
{
	if (!src)
		throw BAD_PARAM ();

	if (!size)
		return 0;

	// Check source data
	if (IsBadReadPtr (src, size))
		throw BAD_PARAM ();

	switch (flags & (Memory::DECOMMIT | Memory::RELEASE)) {

	case 0:
		break;

	case Memory::DECOMMIT:
		if (check_allocated (Page::begin (src), Page::end ((Octet*)src + size)) & ~WIN_MASK_WRITE)
			throw BAD_PARAM ();
		break;

	case Memory::RELEASE:
		check_allocated (Page::begin (src), Page::end ((Octet*)src + size));
		break;

	default:
		throw INV_FLAG ();
	}

	// Destination must be new allocated?
	if ((!dst) || (flags & Memory::ALLOCATE)) {

		// Allocate new block
		flags |= Memory::ALLOCATE;
		flags &= ~Memory::ZERO_INIT;

		// To use sharing, source and destination must have same offset from line begin
		// TODO: Not in all cases it will be optimal (small pieces across page boundaries). 

		UWord line_offset = (UWord)src & (LINE_SIZE - 1);

		if (dst) {  // Allocation address explicitly specified

			if (((UWord)dst & (LINE_SIZE - 1)) == line_offset) {

				UWord page_offset = line_offset & (PAGE_SIZE - 1);

				if (!reserve (size + page_offset, flags, Line::begin (dst))) {

					if (flags & Memory::EXACTLY)
						return 0;

					dst = 0;
				}

			} else
				dst = 0;
		}

		if (!dst) {

			if (flags & Memory::EXACTLY)
				throw BAD_PARAM ();

			// Allocate block with same alignment as source
			Line* dst_begin_line = reserve (size + line_offset, flags);
			dst = dst_begin_line->pages->bytes + line_offset;
		}

	} else {

		// Validate destination block

		UWord dst_protect_mask;
		if (flags & Memory::READ_ONLY)
			dst_protect_mask = ~WIN_MASK_READ;
		else
			dst_protect_mask = ~WIN_MASK_WRITE;

		if (check_allocated (Page::begin (dst), Page::end ((Octet*)dst + size)) & dst_protect_mask)
			throw BAD_PARAM (); // some destination lines allocated with different protection
	}

	// Do copy
	try {

		Octet* dst_end = (Octet*)dst + size;

		if (dst == src)
			if (flags & Memory::ALLOCATE)
				throw BAD_PARAM ();
			else
				return dst;

		// Always perform copy line-by-line, aligned to destination lines

		if (dst < src) {

			void* dst_begin = dst;
			do {

				Octet* dst_line_end = Line::end (dst_begin)->pages->bytes;
				if (dst_line_end > dst_end)
					dst_line_end = dst_end;

				UWord size = dst_line_end - (Octet*)dst;
				copy_one_line ((Octet*)dst_begin, (Octet*)src, size, flags);

				src = (Octet*)src + size;
				dst_begin = dst_line_end;

			} while (dst_begin < dst_end);

		} else {

			do {

				Octet* dst_line_begin = Line::begin (dst_end - 1)->pages->bytes;

				if (dst_line_begin < (Octet*)dst)
					dst_line_begin = (Octet*)dst;
				Octet* src_line_begin = (Octet*)src + (dst_line_begin - (Octet*)dst);

				UWord size = dst_end - dst_line_begin;
				copy_one_line (dst_line_begin, src_line_begin, size, flags);

				dst_end = dst_line_begin;

			} while (dst_end > dst);

		}

	} catch (...) {

		if (flags & Memory::ALLOCATE)
			release (Line::begin (dst), Line::end ((Octet*)dst + size));

		throw;
	}

	if (Memory::RELEASE == (flags & Memory::RELEASE)) {

		// Release memory. DECOMMIT implemented inside copy_one_line.

		Line* release_begin = Line::begin (src);
		Line* release_end = Line::end ((Octet*)src + size);

		if (dst < src) {
			Line* dst_end = Line::end ((Octet*)dst + size);
			if (release_begin < dst_end)
				release_begin = dst_end;
		} else {
			Line* dst_begin = Line::begin (dst);
			if (release_end > dst_begin)
				release_end = dst_begin;
		}

		if (release_begin < release_end)
			release (release_begin, release_end);
	}

	return dst;
}

inline void MemoryWindows::release (Pointer src, UWord size)
{
	if (!src)
		return;

	if (!size)
		return;

	Line* begin = Line::begin (src);
	Line* end = Line::end ((Octet*)src + size);
	check_allocated (begin->pages, end->pages);

	release (begin, end);
}

inline void MemoryWindows::commit (Pointer dst, UWord size)
{
	// Check parameters

	if (!size)
		return;

	if (!dst)
		throw BAD_PARAM ();

	// Align to pages

	Page* begin = Page::begin (dst);
	Page* end = Page::end ((Octet*)dst + size);

	// Check memory state

	if (check_allocated (begin, end) & ~WIN_MASK_WRITE)
		throw BAD_PARAM (); // Some lines not read-write

	// Commit
	commit (begin, end, Memory::ZERO_INIT);
}

inline void MemoryWindows::decommit (Pointer dst, UWord size)
{
	if (!dst)
		throw BAD_PARAM ();

	// Align to pages

	Page* begin = Page::end (dst);
	Page* end = Page::begin ((Octet*)dst + size);

	if (begin >= end)
		return;

	// Check current state

	if (check_allocated (begin, end) & ~WIN_MASK_WRITE)
		throw BAD_PARAM ();

	decommit (begin, end);
}

inline Word MemoryWindows::query (Pointer p, Memory::QueryParam q)
{
	switch (q) {

	case Memory::ALLOCATION_SPACE_BEGIN:
		{
			SYSTEM_INFO sysinfo;
			GetSystemInfo (&sysinfo);
			return (Word)sysinfo.lpMinimumApplicationAddress;
		}

	case Memory::ALLOCATION_SPACE_END:
		return (Word)sm_data.address_space_end;

	case Memory::ALLOCATION_UNIT:
	case Memory::SHARING_UNIT:
	case Memory::GRANULARITY:
		return LINE_SIZE;

	case Memory::PROTECTION_UNIT:
		return PAGE_SIZE;

	case Memory::SHARING_ASSOCIATIVITY:
		return 1;

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

#endif  // _WIN_MEMORY_H_
