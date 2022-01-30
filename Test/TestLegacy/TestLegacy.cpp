#include <Nirvana/Nirvana.h>
#include <Nirvana/Legacy.h>
#include <gtest/gtest.h>

// Test for the Nirvana::Legacy API

using namespace Nirvana;
using namespace Nirvana::Legacy;
using namespace CORBA;

namespace TestLegacy {

class TestLegacy :
	public ::testing::Test
{
protected:
	TestLegacy ()
	{
	}

	virtual ~TestLegacy ()
	{
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		ASSERT_TRUE (g_system->is_legacy_mode ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}
};

TEST_F (TestLegacy, Mutex)
{
	Mutex::_ref_type mtx = g_system->create_mutex ();
	ASSERT_TRUE (mtx);
	mtx->lock ();
	EXPECT_NO_THROW (mtx->unlock ());
	EXPECT_THROW (mtx->unlock (), BAD_INV_ORDER);
}

TEST_F (TestLegacy, Yield)
{
	EXPECT_FALSE (g_system->yield ());
}

}
