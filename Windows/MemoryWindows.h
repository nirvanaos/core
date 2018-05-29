#ifndef NIRVANA_MEMORYWINDOWS_H_
#define NIRVANA_MEMORYWINDOWS_H_

#include <Memory.h>
#include <Windows.h>

namespace Nirvana {

using namespace ::CORBA;
using namespace ::CORBA::Nirvana;

class MemoryWindows
{
	static const ULong PAGE_SIZE = 4096;
	static const ULong LINE_SIZE = 16 * PAGE_SIZE;

public:
	static void initialize ()
	{
		sm_data.initialize ();
	}

	static Pointer allocate (Pointer dst, UWord size, Flags flags)
	{
		if (!size)
			throw BAD_PARAM ();

		if ((Memory::READ_ONLY & flags) && !(Memory::RESERVED & flags))
			throw INV_FLAG ();
		
		Octet* begin, *end;

		if (dst) {
			begin = (Octet*)((UWord)dst & ~(LINE_SIZE - 1));
			if (flags & Memory::EXACTLY)
				end = (Octet*)dst + size;
			else
				end = begin + size;
			end = (Octet*)(((UWord)end + (LINE_SIZE - 1)) & ~(LINE_SIZE - 1));

			if (!VirtualAlloc (begin, end - begin, MEM_RESERVE, PAGE_NOACCESS))
				if (flags & Memory::EXACTLY)
					return 0;	// No exception here!
				else
					begin = 0;
			else {
				(UWord)begin / line
			}
		} else {
			if (flags & Memory::EXACTLY)
				throw INV_FLAG ();
		}
	}

private:
	static BOOL handles_equal (HANDLE h0, HANDLE h1)
	{
		if (sm_data.handles_equal)
			return (*sm_data.handles_equal) (h0, h1);
		else
			return h0 == h1;
	}

	static HANDLE new_line ()
	{
		HANDLE mh = CreateFileMapping (0, 0, PAGE_READWRITE | SEC_RESERVE, 0, LINE_SIZE, 0);
	}

private:
	struct Line
	{ // Currently contains only mapping handle. But we create struct for possible future extensions.
		HANDLE mapping;
	};

	typedef BOOL (__stdcall* HandlesEqual) (HANDLE h0, HANDLE h1);

	static struct Data
	{
		void* address_space_begin;
		void* address_space_end;
		Line* lines;
		HandlesEqual handles_equal;

		void initialize ()
		{
			// Obtain system parameters
			SYSTEM_INFO sysinfo;
			GetSystemInfo (&sysinfo);

			// Check architecture
			if ((PAGE_SIZE != sysinfo.dwPageSize) || (LINE_SIZE != sysinfo.dwAllocationGranularity))
				throw INITIALIZE ();

			address_space_begin = sysinfo.lpMinimumApplicationAddress;
			address_space_end = sysinfo.lpMaximumApplicationAddress;

			size_t max_lines = ((char*)sysinfo.lpMaximumApplicationAddress - (char*)sysinfo.lpMinimumApplicationAddress) / LINE_SIZE;

			// Reserve lines table
			if (!(lines = (Line*)VirtualAlloc (0, sizeof (Line) * max_lines, MEM_RESERVE, PAGE_NOACCESS)))
				throw INITIALIZE ();

			// CompareObjectHandles supported on Windows 10 and later. Missing this function is not fatal, just influents quality of service.
			handles_equal = (HandlesEqual)GetProcAddress (LoadLibraryW (L"Kernelbase.dll"), "CompareObjectHandles");
		}
	} 
	sm_data;
};

}

#endif
