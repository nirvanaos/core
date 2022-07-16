#include <Nirvana/Nirvana.h>
#include <gtest/gtest.h>

using namespace Nirvana;
using namespace std;

// Test for the Nirvana::System interface

namespace TestSystem {

class TestSystem :
	public ::testing::Test
{
protected:
	TestSystem ()
	{
	}

	virtual ~TestSystem ()
	{
	}

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

TEST_F (TestSystem, HeapFactory)
{
	static const size_t GRANULARITY = 128;
	static const size_t BLOCK_SIZE = GRANULARITY * 128;
	static const size_t COUNT = 1024 * 1024 * 4 / 16 * GRANULARITY / BLOCK_SIZE;
	void* blocks [COUNT];
	Memory::_ref_type heap = g_system->create_heap (GRANULARITY);
	EXPECT_EQ (GRANULARITY, heap->query (0, Memory::QueryParam::ALLOCATION_UNIT));

	for (int i = 0; i < COUNT; ++i) {
		size_t cb = BLOCK_SIZE;
		blocks [i] = heap->allocate (nullptr, cb, 0);
		ASSERT_TRUE (blocks [i]);
		UIntPtr au = heap->query (blocks [i], Memory::QueryParam::ALLOCATION_UNIT);
		ASSERT_EQ (GRANULARITY, au);
	}

	for (int i = COUNT - 1; i >= 0; --i) {
		heap->release (blocks [i], BLOCK_SIZE);
	}
}

TEST_F (TestSystem, Yield)
{
	EXPECT_FALSE (g_system->yield ());
}

}
