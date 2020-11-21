#include <core.h>
#include <Port/Memory.h>
#include <gtest/gtest.h>

namespace TestMemory {

using namespace ::Nirvana;
using namespace ::Nirvana::Core;
using namespace ::CORBA;

class TestMemory :
	public ::testing::Test
{
protected:
	TestMemory ()
	{}

	virtual ~TestMemory ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}
};

TEST_F (TestMemory, Allocate)
{
	size_t BLOCK_SIZE = 0x10000000;	// 256M

	static const size_t ITER_CNT = 2;
	static const UWord iter_flags [ITER_CNT] = {
		Memory::RESERVED,
		0
	};
	for (int iteration = 0; iteration < ITER_CNT; ++iteration) {

		UWord flags = iter_flags [iteration];

		// Allocate and release memory.
		uint8_t* block = (uint8_t*)Port::Memory::allocate (0, BLOCK_SIZE, flags);
		ASSERT_TRUE (block);
		Port::Memory::release (block, BLOCK_SIZE);

		flags |= Memory::EXACTLY;

		// Allocate memory at the same range.
		ASSERT_EQ (block, (uint8_t*)Port::Memory::allocate (block, BLOCK_SIZE, flags));
		
		// Release the first half.
		Port::Memory::release (block, BLOCK_SIZE / 2);
		
		// Release the second half.
		Port::Memory::release (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2);

		// Allocate the range again.
		ASSERT_EQ (block, (uint8_t*)Port::Memory::allocate (block, BLOCK_SIZE, flags));
		
		// Release the second half.
		Port::Memory::release (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2);
		
		// Release the first half.
		Port::Memory::release (block, BLOCK_SIZE / 2);

		// Allocate the range again.
		ASSERT_EQ (block, (uint8_t*)Port::Memory::allocate (block, BLOCK_SIZE, flags));

		// Release half at center
		Port::Memory::release (block + BLOCK_SIZE / 4, BLOCK_SIZE / 2);
		
		// Release the first quarter.
		Port::Memory::release (block, BLOCK_SIZE / 4);
		
		// Release the last quarter.
		Port::Memory::release (block + BLOCK_SIZE / 4 * 3, BLOCK_SIZE / 4);

		// Allocate the first half.
		ASSERT_EQ (block, (uint8_t*)Port::Memory::allocate (block, BLOCK_SIZE / 2, flags));
		
		// Allocate the second half.
		ASSERT_EQ (Port::Memory::allocate (block + BLOCK_SIZE / 2, BLOCK_SIZE / 2, flags), block + BLOCK_SIZE / 2);
		
		// Release all range
		Port::Memory::release (block, BLOCK_SIZE);

		// Allocate and release range to check that it is free.
		ASSERT_EQ (block, (uint8_t*)Port::Memory::allocate (block, BLOCK_SIZE, flags));
		Port::Memory::release (block, BLOCK_SIZE);
	}
}

TEST_F (TestMemory, Commit)
{
	size_t BLOCK_SIZE = 0x20000000;	// 512M
	uint8_t* block = (uint8_t*)Port::Memory::allocate (0, BLOCK_SIZE, Memory::READ_WRITE | Memory::RESERVED);
	ASSERT_TRUE (block);
	
	EXPECT_FALSE (Port::Memory::is_writable (block, 1));

	Port::Memory::commit (block, BLOCK_SIZE);
	for (int* p = (int*)block, *end = (int*)(block + BLOCK_SIZE); p < end; ++p)
		*p = rand ();
	
	EXPECT_TRUE (Port::Memory::is_private (block, BLOCK_SIZE));

	Port::Memory::decommit (block, BLOCK_SIZE);
	Port::Memory::decommit (block, BLOCK_SIZE);

	Port::Memory::commit (block, BLOCK_SIZE);
	Port::Memory::commit (block, BLOCK_SIZE);

	Port::Memory::release (block, BLOCK_SIZE);
}

// Test the sharing of the large memory block.
TEST_F (TestMemory, Share)
{
	size_t BLOCK_SIZE = 0x20000000;	// 512M
	uint8_t* block = (uint8_t*)Port::Memory::allocate (0, BLOCK_SIZE, 0);
	ASSERT_TRUE (block);
	uint8_t* end = block + BLOCK_SIZE;

	uint8_t b = 0;
	for (uint8_t* p = block; p < end; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 1);

	EXPECT_TRUE (Port::Memory::is_private (block, BLOCK_SIZE));

	uint8_t* sblock = (uint8_t*)Port::Memory::copy (0, block, BLOCK_SIZE, 0);
	ASSERT_TRUE (sblock);
	uint8_t* send = sblock + BLOCK_SIZE;
	EXPECT_EQ (sblock [1], 1);

	EXPECT_FALSE (Port::Memory::is_private (block, BLOCK_SIZE));
	EXPECT_FALSE (Port::Memory::is_private (sblock, BLOCK_SIZE));
	EXPECT_TRUE (Port::Memory::is_copy (sblock, block, BLOCK_SIZE));
	EXPECT_TRUE (Port::Memory::is_copy (block, sblock, BLOCK_SIZE));

	EXPECT_EQ (sblock, (uint8_t*)Port::Memory::copy (sblock, block, BLOCK_SIZE, 0));
	EXPECT_EQ (sblock [1], 1);

	EXPECT_FALSE (Port::Memory::is_private (block, BLOCK_SIZE));
	EXPECT_FALSE (Port::Memory::is_private (sblock, BLOCK_SIZE));
	EXPECT_TRUE (Port::Memory::is_copy (sblock, block, BLOCK_SIZE));
	EXPECT_TRUE (Port::Memory::is_copy (block, sblock, BLOCK_SIZE));

	b = 1;
	for (uint8_t* p = block; p < end; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 2);
	EXPECT_EQ (sblock [1], 1);
	
	EXPECT_TRUE (Port::Memory::is_private (block, BLOCK_SIZE));
	EXPECT_FALSE (Port::Memory::is_copy (sblock, block, BLOCK_SIZE));

	b = 2;
	for (uint8_t* p = sblock; p < send; ++p)
		*p = b++;
	EXPECT_EQ (block [1], 2);
	EXPECT_EQ (sblock [1], 3);
	
	EXPECT_TRUE (Port::Memory::is_private (sblock, BLOCK_SIZE));

	EXPECT_EQ (block, Port::Memory::copy (block, sblock, BLOCK_SIZE, 0));
	EXPECT_EQ (block [1], 3);

	EXPECT_FALSE (Port::Memory::is_private (block, BLOCK_SIZE));
	EXPECT_FALSE (Port::Memory::is_private (sblock, BLOCK_SIZE));
	EXPECT_TRUE (Port::Memory::is_copy (sblock, block, BLOCK_SIZE));
	EXPECT_TRUE (Port::Memory::is_copy (block, sblock, BLOCK_SIZE));

	Port::Memory::release (block, BLOCK_SIZE);
	Port::Memory::release (sblock, BLOCK_SIZE);
}

TEST_F (TestMemory, Move)
{
	size_t BLOCK_SIZE = 0x20000000;	// 512M
	size_t SHIFT = Port::Memory::SHARING_ASSOCIATIVITY;
	// Allocate block.
	int* block = (int*)Port::Memory::allocate (0, BLOCK_SIZE + SHIFT, Memory::ZERO_INIT | Memory::RESERVED);
	ASSERT_TRUE (block);
	Port::Memory::commit (block, BLOCK_SIZE);

	int i = 0;
	for (int* p = block, *end = block + BLOCK_SIZE / sizeof (int); p != end; ++p)
		*p = ++i;

	// Shift block right on SHIFT
	int* shifted = (int*)Port::Memory::copy (block + SHIFT / sizeof (int), block, BLOCK_SIZE, Memory::EXACTLY | Memory::RELEASE);
	EXPECT_EQ (shifted, block + SHIFT / sizeof (int));
	i = 0;
	for (int* p = shifted, *end = shifted + BLOCK_SIZE / sizeof (int); p != end; ++p)
		EXPECT_EQ (*p, ++i);
	EXPECT_TRUE (Port::Memory::is_private (shifted, BLOCK_SIZE));

	// Allocate region to ensure that it is free.
	EXPECT_TRUE (Port::Memory::allocate (block, SHIFT, Memory::RESERVED | Memory::EXACTLY));
	Port::Memory::release (block, SHIFT);

	// Shift it back.
	EXPECT_EQ (block, (int*)Port::Memory::copy (block, shifted, BLOCK_SIZE, Memory::ALLOCATE | Memory::EXACTLY | Memory::RELEASE));
	i = 0;
	for (int* p = block, *end = block + BLOCK_SIZE / sizeof (int); p != end; ++p)
		EXPECT_EQ (*p, ++i);
	EXPECT_TRUE (Port::Memory::is_private (block, BLOCK_SIZE));

	// Allocate region to ensure that it is free.
	EXPECT_TRUE (Port::Memory::allocate (block + BLOCK_SIZE / sizeof (int), SHIFT, Memory::RESERVED | Memory::EXACTLY));
	Port::Memory::release (block + BLOCK_SIZE / sizeof (int), SHIFT);

	Port::Memory::release (block, BLOCK_SIZE);
}

TEST_F (TestMemory, SmallBlock)
{
	int* block = (int*)Port::Memory::allocate (0, sizeof (int), Memory::ZERO_INIT);
	ASSERT_TRUE (block);
	EXPECT_TRUE (Port::Memory::is_private (block, sizeof (int)));
	*block = 1;
	{
		int* copy = (int*)Port::Memory::copy (0, block, sizeof (int), 0);
		ASSERT_TRUE (copy);
		EXPECT_EQ (*copy, *block);
		EXPECT_TRUE (Port::Memory::is_readable (copy, sizeof (int)));
		EXPECT_TRUE (Port::Memory::is_writable (copy, sizeof (int)));
		EXPECT_TRUE (Port::Memory::is_copy (copy, block, sizeof (int)));
		EXPECT_FALSE (Port::Memory::is_private (block, sizeof (int)));
		*copy = 2;
		EXPECT_EQ (*block, 1);
		Port::Memory::release (copy, sizeof (int));
	}
	{
		int* copy = (int*)Port::Memory::copy (0, block, sizeof (int), Memory::READ_ONLY);
		ASSERT_TRUE (copy);
		EXPECT_EQ (*copy, *block);
		EXPECT_TRUE (Port::Memory::is_readable (copy, sizeof (int)));
		EXPECT_FALSE (Port::Memory::is_writable (copy, sizeof (int)));
		EXPECT_TRUE (Port::Memory::is_copy (copy, block, sizeof (int)));
		EXPECT_FALSE (Port::Memory::is_writable (copy, sizeof (int)));
		Port::Memory::release (copy, sizeof (int));
	}
	Port::Memory::decommit (block, Port::Memory::FIXED_COMMIT_UNIT);
	Port::Memory::commit (block, sizeof (int));
	*block = 1;
	{
		EXPECT_TRUE (Port::Memory::is_private (block, sizeof (int)));
		int* copy = (int*)Port::Memory::copy (0, block, Port::Memory::FIXED_COMMIT_UNIT, Memory::DECOMMIT);
		EXPECT_EQ (*copy, 1);
		EXPECT_TRUE (Port::Memory::is_readable (copy, sizeof (int)));
		EXPECT_TRUE (Port::Memory::is_writable (copy, sizeof (int)));
		EXPECT_FALSE (Port::Memory::is_readable (block, sizeof (int)));
		EXPECT_FALSE (Port::Memory::is_writable (block, sizeof (int)));
		EXPECT_FALSE (Port::Memory::is_writable (block, sizeof (int)));
		Port::Memory::commit (block, sizeof (int));
		*block = 2;
		EXPECT_TRUE (Port::Memory::is_private (block, sizeof (int)));
		EXPECT_TRUE (Port::Memory::is_private (copy, sizeof (int)));
		EXPECT_FALSE (Port::Memory::is_copy (copy, block, sizeof (int)));
		Port::Memory::release (copy, sizeof (int));
	}
	{
		int* copy = (int*)Port::Memory::copy (0, block, sizeof (int), Memory::RELEASE);
		ASSERT_EQ (copy, block);
	}
	Port::Memory::release (block, sizeof (int));
}

TEST_F (TestMemory, NotShared)
{
	static const char test_const [] = "test";
	char* copy = (char*)Port::Memory::copy (0, (void*)test_const, sizeof (test_const), Memory::ALLOCATE);
	static char test [sizeof (test_const)];
	Port::Memory::copy (test, copy, sizeof (test_const), 0);
	EXPECT_STREQ (test, test_const);
	Port::Memory::release (copy, sizeof (test_const));
}

}
