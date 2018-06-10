#include "../MemoryWindows.h"
#include <gtest/gtest.h>

namespace unittests {

using namespace ::Nirvana;
using namespace ::Nirvana::Windows;

class TestMemoryWindows :
	public ::testing::Test
{
protected:
	TestMemoryWindows ()
	{}

	virtual ~TestMemoryWindows ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		MemoryWindows::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		MemoryWindows::terminate ();
	}
};

TEST_F (TestMemoryWindows, Allocate)
{
	size_t BLOCK_SIZE = 0x10000000;	// 256M

	static const size_t ITER_CNT = 3;
	static const Flags iter_flags [ITER_CNT] = {
		Memory::READ_WRITE | Memory::RESERVED,
		Memory::READ_WRITE,
		Memory::READ_ONLY | Memory::RESERVED,
	};
	for (int iteration = 0; iteration < ITER_CNT; ++iteration) {

		LONG flags = iter_flags [iteration];

		BYTE* block = (BYTE*)MemoryWindows::allocate (0, BLOCK_SIZE, flags);
		ASSERT_TRUE (block);
		MemoryWindows::release (block, BLOCK_SIZE);

		flags |= Memory::EXACTLY;

		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE, flags));
		MemoryWindows::release (block, BLOCK_SIZE / 2);
		MemoryWindows::release (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2);

		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE, flags));
		MemoryWindows::release (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2);
		MemoryWindows::release (block, BLOCK_SIZE / 2);

		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE, flags));
		MemoryWindows::release (block + BLOCK_SIZE / 4, BLOCK_SIZE / 2);
		MemoryWindows::release (block, BLOCK_SIZE / 4);
		MemoryWindows::release (block + BLOCK_SIZE / 4 * 3, BLOCK_SIZE / 4);

		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE / 2, flags));
		ASSERT_EQ (MemoryWindows::allocate (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2, flags), block + BLOCK_SIZE / 2);
		MemoryWindows::release (block, BLOCK_SIZE);

		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE, flags));
		MemoryWindows::release (block, BLOCK_SIZE);
	}
}

TEST_F (TestMemoryWindows, Commit)
{
	size_t BLOCK_SIZE = 0x20000000;	// 512M
	BYTE* block = (BYTE*)MemoryWindows::allocate (0, BLOCK_SIZE, Memory::READ_WRITE | Memory::RESERVED);
	ASSERT_TRUE (block);
	BYTE* end = block + BLOCK_SIZE;

	MemoryWindows::commit (block, BLOCK_SIZE);
	for (int* p = (int*)block, *end = (int*)(block + BLOCK_SIZE); p < end; ++p)
		*p = rand ();

	MemoryWindows::decommit (block, BLOCK_SIZE);
	MemoryWindows::decommit (block, BLOCK_SIZE);

	MemoryWindows::commit (block, BLOCK_SIZE);
	MemoryWindows::commit (block, BLOCK_SIZE);

	MemoryWindows::release (block, BLOCK_SIZE);
}

/*
TEST_F (TestMemoryWindows, Share)
{
	size_t BLOCK_SIZE = 0x20000000;	// 512M
	BYTE* block = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0);
	ASSERT_TRUE (block);
	BYTE* end = block + BLOCK_SIZE;
	for (BYTE* p = block; p < end; p += ALLOCATION_GRANULARITY) {
		process_memory.map (p);
		EXPECT_TRUE (VirtualAlloc (p, ALLOCATION_GRANULARITY, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
	}

	BYTE b = 0;
	for (BYTE* p = block; p < end; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 1);

	BYTE* sblock = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0);
	ASSERT_TRUE (sblock);
	BYTE* send = sblock + BLOCK_SIZE;

	LARGE_INTEGER perf_begin, perf_end;
	QueryPerformanceCounter (&perf_begin);

	for (BYTE* p = block; p < end; p += ALLOCATION_GRANULARITY) {
		DWORD old;
		EXPECT_TRUE (VirtualProtect (p, ALLOCATION_GRANULARITY, PAGE_WRITECOPY, &old));
	}

	for (BYTE* p = block, *sp = sblock; p < end; p += ALLOCATION_GRANULARITY, sp += ALLOCATION_GRANULARITY)
		process_memory.map (sp, process_memory.block ((const void*)p).mapping);

	QueryPerformanceCounter (&perf_end);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency (&freq);
	int MBps = (int)((LONGLONG)BLOCK_SIZE * freq.QuadPart / (perf_end.QuadPart - perf_begin.QuadPart) / (1024 * 1024));
	::std::cerr << "Sharing MB/s " << MBps << ::std::endl;

	EXPECT_EQ (sblock [1], 1);

	b = 1;
	for (BYTE* p = block; p < end; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 2);
	EXPECT_EQ (sblock [1], 1);

	b = 2;
	for (BYTE* p = sblock; p < send; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 2);
	EXPECT_EQ (sblock [1], 3);

	process_memory.release (block, BLOCK_SIZE);
	process_memory.release (sblock, BLOCK_SIZE);

	block = (BYTE*)malloc (BLOCK_SIZE);
	ASSERT_TRUE (block);
	end = block + BLOCK_SIZE;
	b = 0;
	sblock = (BYTE*)malloc (BLOCK_SIZE);
	ASSERT_TRUE (sblock);
	for (BYTE* p = block; p < end; ++p)
		*p = b++;
	QueryPerformanceCounter (&perf_begin);
	memcpy (sblock, block, BLOCK_SIZE);
	QueryPerformanceCounter (&perf_end);
	int MBpsPlain = (int)((LONGLONG)BLOCK_SIZE * freq.QuadPart / (perf_end.QuadPart - perf_begin.QuadPart) / (1024 * 1024));
	::std::cerr << "Plain MB/s " << MBpsPlain << ::std::endl;

	free (block);
	free (sblock);
}
*/
}
