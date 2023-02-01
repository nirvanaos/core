#include <Nirvana/Nirvana.h>
#include <gtest/gtest.h>

using namespace Nirvana;

// Test for the Nirvana::System interface

extern void writemem (void* p);

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

TEST_F (TestSystem, AccessViolation)
{
	Memory::_ref_type heap = g_system->create_heap (0);
	size_t cb = (size_t)heap->query (nullptr, Memory::QueryParam::PROTECTION_UNIT);
	if (!cb)
		return;

	void* p = heap->allocate (0, cb, Memory::RESERVED);
	bool OK = false;
	int minor = 0;
	try {
		writemem (p);
	} catch (const CORBA::ACCESS_VIOLATION& ex) {
		OK = true;
		minor = ex.minor ();
	}
	heap->release (p, cb);
	EXPECT_TRUE (OK);
	EXPECT_EQ (SEGV_MAPERR, minor);

	p = heap->allocate (0, cb, 0);
	void* p1 = heap->copy (nullptr, p, cb, Memory::READ_ONLY);
	OK = false;
	try {
		writemem (p1);
	} catch (const CORBA::ACCESS_VIOLATION& ex) {
		OK = true;
		minor = ex.minor ();
	}
	heap->release (p, cb);
	heap->release (p1, cb);
	EXPECT_TRUE (OK);
	EXPECT_EQ (SEGV_ACCERR, minor);
}

TEST_F (TestSystem, Yield)
{
	EXPECT_FALSE (g_system->yield ());
}

}
