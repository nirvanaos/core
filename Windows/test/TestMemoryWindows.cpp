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

	static const size_t ITER_CNT = 2;
	static const Flags iter_flags [ITER_CNT] = {
		Memory::READ_WRITE | Memory::RESERVED,
		Memory::READ_WRITE
	};
	for (int iteration = 0; iteration < ITER_CNT; ++iteration) {

		LONG flags = iter_flags [iteration];

		// Allocate and release memory.
		BYTE* block = (BYTE*)MemoryWindows::allocate (0, BLOCK_SIZE, flags);
		ASSERT_TRUE (block);
		MemoryWindows::release (block, BLOCK_SIZE);

		flags |= Memory::EXACTLY;

		// Allocate memory at the same range.
		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE, flags));
		
		// Release the first half.
		MemoryWindows::release (block, BLOCK_SIZE / 2);
		
		// Release the second half.
		MemoryWindows::release (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2);

		// Allocate the range again.
		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE, flags));
		
		// Release the second half.
		MemoryWindows::release (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2);
		
		// Release the first half.
		MemoryWindows::release (block, BLOCK_SIZE / 2);

		// Allocate the range again.
		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE, flags));

		// Release half at center
		MemoryWindows::release (block + BLOCK_SIZE / 4, BLOCK_SIZE / 2);
		
		// Release the first quarter.
		MemoryWindows::release (block, BLOCK_SIZE / 4);
		
		// Release the last quarter.
		MemoryWindows::release (block + BLOCK_SIZE / 4 * 3, BLOCK_SIZE / 4);

		// Allocate the first half.
		ASSERT_EQ (block, (BYTE*)MemoryWindows::allocate (block, BLOCK_SIZE / 2, flags));
		
		// Allocate the second half.
		ASSERT_EQ (MemoryWindows::allocate (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2, flags), block + BLOCK_SIZE / 2);
		
		// Release all range
		MemoryWindows::release (block, BLOCK_SIZE);

		// Allocate and release range to check that it is free.
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
	
	EXPECT_THROW (*block = 1, MEM_NOT_COMMITTED);

	MemoryWindows::commit (block, BLOCK_SIZE);
	for (int* p = (int*)block, *end = (int*)(block + BLOCK_SIZE); p < end; ++p)
		*p = rand ();
	
	EXPECT_TRUE (MemoryWindows::is_private (block, BLOCK_SIZE));

	MemoryWindows::decommit (block, BLOCK_SIZE);
	MemoryWindows::decommit (block, BLOCK_SIZE);

	MemoryWindows::commit (block, BLOCK_SIZE);
	MemoryWindows::commit (block, BLOCK_SIZE);

	MemoryWindows::release (block, BLOCK_SIZE);
}

// Test the sharing of the large memory block.
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

	EXPECT_TRUE (MemoryWindows::is_private (block, BLOCK_SIZE));

	BYTE* sblock = (BYTE*)MemoryWindows::copy (0, block, BLOCK_SIZE, 0);
	ASSERT_TRUE (sblock);
	BYTE* send = sblock + BLOCK_SIZE;
	EXPECT_EQ (sblock [1], 1);

	EXPECT_FALSE (MemoryWindows::is_private (block, BLOCK_SIZE));
	EXPECT_FALSE (MemoryWindows::is_private (sblock, BLOCK_SIZE));
	EXPECT_TRUE (MemoryWindows::is_copy (sblock, block, BLOCK_SIZE));
	EXPECT_TRUE (MemoryWindows::is_copy (block, sblock, BLOCK_SIZE));

	EXPECT_EQ (sblock, (BYTE*)MemoryWindows::copy (sblock, block, BLOCK_SIZE, 0));
	EXPECT_EQ (sblock [1], 1);

	EXPECT_FALSE (MemoryWindows::is_private (block, BLOCK_SIZE));
	EXPECT_FALSE (MemoryWindows::is_private (sblock, BLOCK_SIZE));
	EXPECT_TRUE (MemoryWindows::is_copy (sblock, block, BLOCK_SIZE));
	EXPECT_TRUE (MemoryWindows::is_copy (block, sblock, BLOCK_SIZE));

	b = 1;
	for (BYTE* p = block; p < end; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 2);
	EXPECT_EQ (sblock [1], 1);
	
	EXPECT_TRUE (MemoryWindows::is_private (block, BLOCK_SIZE));
	EXPECT_FALSE (MemoryWindows::is_copy (sblock, block, BLOCK_SIZE));

	b = 2;
	for (BYTE* p = sblock; p < send; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 2);
	EXPECT_EQ (sblock [1], 3);
	
	EXPECT_TRUE (MemoryWindows::is_private (sblock, BLOCK_SIZE));

	EXPECT_EQ (block, MemoryWindows::copy (block, sblock, BLOCK_SIZE, 0));
	EXPECT_EQ (block [1], 3);

	EXPECT_FALSE (MemoryWindows::is_private (block, BLOCK_SIZE));
	EXPECT_FALSE (MemoryWindows::is_private (sblock, BLOCK_SIZE));
	EXPECT_TRUE (MemoryWindows::is_copy (sblock, block, BLOCK_SIZE));
	EXPECT_TRUE (MemoryWindows::is_copy (block, sblock, BLOCK_SIZE));

	MemoryWindows::release (block, BLOCK_SIZE);
	MemoryWindows::release (sblock, BLOCK_SIZE);
}

TEST_F (TestMemoryWindows, Move)
{
	size_t BLOCK_SIZE = 0x20000000;	// 512M
	size_t SHIFT = ALLOCATION_GRANULARITY;
	// Allocate block.
	int* block = (int*)MemoryWindows::allocate (0, BLOCK_SIZE + SHIFT, Memory::ZERO_INIT | Memory::RESERVED);
	ASSERT_TRUE (block);
	MemoryWindows::commit (block, BLOCK_SIZE);

	int i = 0;
	for (int* p = block, *end = block + BLOCK_SIZE / sizeof (int); p != end; ++p)
		*p = ++i;

	// Shift block right on SHIFT
	int* shifted = (int*)MemoryWindows::copy (block + SHIFT / sizeof (int), block, BLOCK_SIZE, Memory::EXACTLY | Memory::RELEASE);
	EXPECT_EQ (shifted, block + SHIFT / sizeof (int));
	i = 0;
	for (int* p = shifted, *end = shifted + BLOCK_SIZE / sizeof (int); p != end; ++p)
		EXPECT_EQ (*p, ++i);
	EXPECT_TRUE (MemoryWindows::is_private (shifted, BLOCK_SIZE));

	// Allocate region to ensure that it is free.
	EXPECT_TRUE (MemoryWindows::allocate (block, SHIFT, Memory::RESERVED | Memory::EXACTLY));
	MemoryWindows::release (block, SHIFT);

	// Shift it back.
	EXPECT_EQ (block, (int*)MemoryWindows::copy (block, shifted, BLOCK_SIZE, Memory::ALLOCATE | Memory::EXACTLY | Memory::RELEASE));
	i = 0;
	for (int* p = block, *end = block + BLOCK_SIZE / sizeof (int); p != end; ++p)
		EXPECT_EQ (*p, ++i);
	EXPECT_TRUE (MemoryWindows::is_private (block, BLOCK_SIZE));

	// Allocate region to ensure that it is free.
	EXPECT_TRUE (MemoryWindows::allocate (block + BLOCK_SIZE / sizeof (int), SHIFT, Memory::RESERVED | Memory::EXACTLY));
	MemoryWindows::release (block + BLOCK_SIZE / sizeof (int), SHIFT);

	MemoryWindows::release (block, BLOCK_SIZE);
}

TEST_F (TestMemoryWindows, SmallBlock)
{
	int* block = (int*)MemoryWindows::allocate (0, sizeof (int), Memory::ZERO_INIT);
	ASSERT_TRUE (block);
	EXPECT_TRUE (MemoryWindows::is_private (block, sizeof (int)));
	*block = 1;
	{
		int* copy = (int*)MemoryWindows::copy (0, block, sizeof (int), 0);
		ASSERT_TRUE (copy);
		EXPECT_EQ (*copy, *block);
		EXPECT_TRUE (MemoryWindows::is_readable (copy, sizeof (int)));
		EXPECT_TRUE (MemoryWindows::is_writable (copy, sizeof (int)));
		EXPECT_TRUE (MemoryWindows::is_copy (copy, block, sizeof (int)));
		EXPECT_FALSE (MemoryWindows::is_private (block, sizeof (int)));
		*copy = 2;
		EXPECT_EQ (*block, 1);
		MemoryWindows::release (copy, sizeof (int));
	}
	{
		int* copy = (int*)MemoryWindows::copy (0, block, sizeof (int), Memory::READ_ONLY);
		ASSERT_TRUE (copy);
		EXPECT_EQ (*copy, *block);
		EXPECT_TRUE (MemoryWindows::is_readable (copy, sizeof (int)));
		EXPECT_FALSE (MemoryWindows::is_writable (copy, sizeof (int)));
		EXPECT_TRUE (MemoryWindows::is_copy (copy, block, sizeof (int)));
		EXPECT_THROW (*copy = 2, NO_PERMISSION);
		MemoryWindows::release (copy, sizeof (int));
	}
	MemoryWindows::decommit (block, PAGE_SIZE);
	MemoryWindows::commit (block, sizeof (int));
	*block = 1;
	{
		EXPECT_TRUE (MemoryWindows::is_private (block, sizeof (int)));
		int* copy = (int*)MemoryWindows::copy (0, block, PAGE_SIZE, Memory::DECOMMIT);
		EXPECT_EQ (*copy, 1);
		EXPECT_TRUE (MemoryWindows::is_readable (copy, sizeof (int)));
		EXPECT_TRUE (MemoryWindows::is_writable (copy, sizeof (int)));
		EXPECT_FALSE (MemoryWindows::is_readable (block, sizeof (int)));
		EXPECT_FALSE (MemoryWindows::is_writable (block, sizeof (int)));
		EXPECT_THROW (*block = 2, MEM_NOT_COMMITTED);
		MemoryWindows::commit (block, sizeof (int));
		*block = 2;
		EXPECT_TRUE (MemoryWindows::is_private (block, sizeof (int)));
		EXPECT_TRUE (MemoryWindows::is_private (copy, sizeof (int)));
		EXPECT_FALSE (MemoryWindows::is_copy (copy, block, sizeof (int)));
		MemoryWindows::release (copy, sizeof (int));
	}
	{
		int* copy = (int*)MemoryWindows::copy (0, block, sizeof (int), Memory::RELEASE);
		ASSERT_EQ (copy, block);
	}
	MemoryWindows::release (block, sizeof (int));
}

inline NT_TIB* current_TIB ()
{
#ifdef _M_IX86
	return (NT_TIB*)__readfsdword (0x18);
#elif _M_AMD64
	return (NT_TIB*)__readgsqword (0x30);
#else
#error Only x86 and x64 platforms supported.
#endif
}

void stack_test (void* limit, bool first)
{
	BYTE data [4096];
	data [0] = 1;
	MEMORY_BASIC_INFORMATION mbi;
	ASSERT_TRUE (VirtualQuery (data, &mbi, sizeof (mbi)));
	ASSERT_EQ (mbi.Protect, PageState::RW_MAPPED_PRIVATE);
	if (current_TIB ()->StackLimit != limit)
		if (first)
			first = false;
		else
			return;
	stack_test (limit, first);
}

TEST_F (TestMemoryWindows, Stack)
{
	MemoryWindows::ThreadMemory tm;
	stack_test (current_TIB ()->StackLimit, true);
}

}
