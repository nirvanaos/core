#include <Nirvana/Nirvana.h>
#include <gtest/gtest.h>

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

}
