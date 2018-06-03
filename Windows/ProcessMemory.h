#ifndef NIRVANA_WINDOWS_PROCESSMEMORY_H_
#define NIRVANA_WINDOWS_PROCESSMEMORY_H_

#include <ORB.h>
#include "Win32.h"

namespace Nirvana {

class ProcessMemory
{
public:
	struct VirtualLine
	{
		// Currently we are using only one field. But we create structure for possible future extensions.
		HANDLE mapping;
	};

	enum
	{
		WIN_NO_ACCESS = PAGE_NOACCESS,

		WIN_WRITE_RESERVE = PAGE_READWRITE,
		WIN_READ_RESERVE = PAGE_READONLY,

		WIN_WRITE_MAPPED_PRIVATE = PAGE_READWRITE,
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

private:
	static const size_t SECOND_LEVEL_BLOCK = LINE_SIZE / sizeof (VirtualLine);

public:
	void initialize ()
	{
		initialize (GetCurrentProcessId (), GetCurrentProcess ());
	}

	void terminate ();

	const VirtualLine& line (const void* address) const
	{
		size_t idx = (size_t)address / LINE_SIZE;
		assert (idx < m_directory_size);
		const VirtualLine* p;
#ifdef _WIN64
		size_t i0 = idx / SECOND_LEVEL_BLOCK;
		size_t i1 = idx % SECOND_LEVEL_BLOCK;
		p = m_directory [i0] + i1;
#else
		p = m_directory + idx;
#endif
		assert (p->mapping);
		return *p;
	}

	VirtualLine& line (void* address);

	void* reserve (size_t size, LONG flags, void* dst = 0);
	void release (void* dst, size_t size);
	void map (void* dst, HANDLE src = 0);
	void unmap (void* dst);

private:
	void initialize (DWORD process_id, HANDLE process_handle);

private:
	HANDLE m_process;
	HANDLE m_mapping;
#ifdef _WIN64
	VirtualLine** m_directory;
#else
	VirtualLine* m_directory;
#endif
	size_t m_directory_size;
};

}

#endif
