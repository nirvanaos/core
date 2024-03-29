#include "pch.h"
#include "../Source/PreallocatedStack.h"

using namespace Nirvana::Core;

namespace TestPreallocatedStack {

class TestPreallocatedStack :
	public ::testing::Test
{
protected:
	TestPreallocatedStack ()
	{}

	virtual ~TestPreallocatedStack ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		ASSERT_TRUE (Heap::initialize ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Heap::terminate ();
	}
};

TEST_F (TestPreallocatedStack, Test)
{
	PreallocatedStack <int, 16, 16> stack;

	ASSERT_TRUE (stack.empty ());

	for (int i = 0; i < 100; ++i) {
		stack.push (i);
		ASSERT_FALSE (stack.empty ());
		ASSERT_EQ (i, stack.top ());
	}

	for (int i = 99; i >= 0; --i) {
		ASSERT_EQ (i, stack.top ());
		ASSERT_FALSE (stack.empty ());
		stack.pop ();
	}
}

TEST_F (TestPreallocatedStack, Init)
{
	PreallocatedStack <int> stack (1);
	ASSERT_FALSE (stack.empty ());
	EXPECT_EQ (stack.top (), 1);
}

}
