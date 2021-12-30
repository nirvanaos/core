#include <Nirvana/Nirvana.h>
#include "../../Source/FileAccessDirect.h"
#include "../../Source/Synchronized.h"
#include <gtest/gtest.h>

using namespace Nirvana;
using namespace std;

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
		blocks [i] = heap->allocate (0, BLOCK_SIZE, 0);
		ASSERT_TRUE (blocks [i]);
		UIntPtr au = heap->query (blocks [i], Memory::QueryParam::ALLOCATION_UNIT);
		ASSERT_EQ (GRANULARITY, au);
	}

	for (int i = COUNT - 1; i >= 0; --i) {
		heap->release (blocks [i], BLOCK_SIZE);
	}
}
/* Fails!!!
TEST_F (TestSystem, FileAccessDirect)
{
	char file_name [L_tmpnam_s];
	ASSERT_FALSE (tmpnam_s (file_name));
	Nirvana::FileAccessDirect::_ref_type fa;
	SYNC_BEGIN (Core::SyncContext::current ());
	fa = CORBA::make_reference <Nirvana::Core::FileAccessDirect> (file_name, O_CREAT | O_TRUNC)->_this ();
	SYNC_END ();

	EXPECT_EQ (fa->size (), 0);
	vector <uint8_t> buf;
	buf.resize (1, 1);
	fa->write (0, buf);
	fa->flush ();
	fa = nullptr;
}
*/
}
