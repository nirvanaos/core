#include <Nirvana/Nirvana.h>
#include <gtest/gtest.h>
#include <Nirvana/Domains.h>

using namespace Nirvana;

namespace TestESIOP {

class TestESIOP :
	public ::testing::Test
{
protected:
	TestESIOP ()
	{
	}

	virtual ~TestESIOP ()
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

TEST_F (TestESIOP, SysDomain)
{
	SysDomain::_ref_type sd = SysDomain::_narrow (CORBA::g_ORB->resolve_initial_references ("SysDomain"));
	ASSERT_TRUE (sd);
	for (int i = 0; i < 100; ++i) {
		BindInfo bi;
		sd->get_bind_info ("Nirvana/g_dec_calc", PLATFORM, bi);
	}
}


}
