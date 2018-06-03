#include "../ProcessMemory.h"
#include <gtest/gtest.h>

namespace unittests {

using namespace ::Nirvana;

static ProcessMemory process_memory;

class TestProcessMemory :
	public ::testing::Test
{
protected:
	TestProcessMemory ()
	{}

	virtual ~TestProcessMemory ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		process_memory.initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		process_memory.terminate ();
	}
};

TEST_F (TestProcessMemory, Allocate)
{
	size_t BLOCK_SIZE = 0x10000000;	// 256M
	BYTE* block = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0);
	ASSERT_TRUE (block);
	process_memory.release (block, BLOCK_SIZE);

	block = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0, block);
	ASSERT_TRUE (block);
	process_memory.release (block, BLOCK_SIZE / 2);
	process_memory.release (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2);

	block = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0, block);
	ASSERT_TRUE (block);
	process_memory.release (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2);
	process_memory.release (block, BLOCK_SIZE / 2);

	block = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0, block);
	ASSERT_TRUE (block);
	process_memory.release (block + BLOCK_SIZE / 4, BLOCK_SIZE / 2);
	process_memory.release (block, BLOCK_SIZE / 4);
	process_memory.release (block + BLOCK_SIZE / 4 * 3, BLOCK_SIZE / 4);

	block = (BYTE*)process_memory.reserve (BLOCK_SIZE / 2, 0, block);
	ASSERT_TRUE (block);
	ASSERT_EQ (process_memory.reserve (BLOCK_SIZE / 2, 0, block + BLOCK_SIZE / 2), block + BLOCK_SIZE / 2);
	process_memory.release (block, BLOCK_SIZE);
}

TEST_F (TestProcessMemory, Map)
{
	size_t BLOCK_SIZE = 0x10000000;	// 256M
	BYTE* block = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0);
	ASSERT_TRUE (block);

	BYTE* end = block + BLOCK_SIZE;
	for (BYTE* p = block; p < end; p += LINE_SIZE) {
		process_memory.map (p);
		EXPECT_TRUE (VirtualAlloc (p, LINE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	}

	EXPECT_EQ (block [1], 0);

	BYTE b = 0;
	for (BYTE* p = block; p < end; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 1);

	for (BYTE* p = block; p < end; p += LINE_SIZE)
		process_memory.unmap (p);

	for (BYTE* p = block; p < end; p += LINE_SIZE) {
		process_memory.map (p);
		EXPECT_TRUE (VirtualAlloc (p, LINE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	}

	EXPECT_EQ (block [1], 0);

	process_memory.release (block, BLOCK_SIZE);
}

TEST_F (TestProcessMemory, Share)
{
	size_t BLOCK_SIZE = 0x10000000;	// 256M
	BYTE* block = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0);
	ASSERT_TRUE (block);
	BYTE* end = block + BLOCK_SIZE;
	for (BYTE* p = block; p < end; p += LINE_SIZE) {
		process_memory.map (p);
		EXPECT_TRUE (VirtualAlloc (p, LINE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	}

	BYTE b = 0;
	for (BYTE* p = block; p < end; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 1);

	BYTE* sblock = (BYTE*)process_memory.reserve (BLOCK_SIZE, 0);
	ASSERT_TRUE (sblock);
	BYTE* send = sblock + BLOCK_SIZE;

	for (BYTE* p = block; p < end; p += LINE_SIZE) {
		DWORD old;
		EXPECT_TRUE (VirtualProtect (p, LINE_SIZE, PAGE_WRITECOPY, &old));
	}

	for (BYTE* p = block, *sp = sblock; p < end; p += LINE_SIZE, sp += LINE_SIZE)
		process_memory.map (sp, process_memory.line ((const void*)p).mapping);

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
}

}
