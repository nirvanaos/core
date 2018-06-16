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

TEST_F (TestMemoryWindows, Share)
{
	size_t BLOCK_SIZE = 0x20000000;	// 512M
	BYTE* block = (BYTE*)MemoryWindows::allocate (0, BLOCK_SIZE, 0);
	ASSERT_TRUE (block);
	BYTE* end = block + BLOCK_SIZE;

	BYTE b = 0;
	for (BYTE* p = block; p < end; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 1);

	BYTE* sblock = (BYTE*)MemoryWindows::copy (0, block, BLOCK_SIZE, 0);
	ASSERT_TRUE (sblock);
	BYTE* send = sblock + BLOCK_SIZE;
	EXPECT_EQ (sblock [1], 1);

	EXPECT_EQ (sblock, (BYTE*)MemoryWindows::copy (sblock, block, BLOCK_SIZE, 0));
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

	EXPECT_EQ (block, MemoryWindows::copy (block, sblock, BLOCK_SIZE, 0));
	EXPECT_EQ (block [1], 3);

	MemoryWindows::release (block, BLOCK_SIZE);
	MemoryWindows::release (sblock, BLOCK_SIZE);
}

TEST_F (TestMemoryWindows, Stack)
{
	MemoryWindows::ThreadMemory tm;
}

}
