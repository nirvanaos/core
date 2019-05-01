#include <CORBA/String.h>
#include "../Source/Heap.h"
#include "gtest/gtest.h"

namespace TestString {

using namespace CORBA;

template <typename C>
class TestString :
	public ::testing::Test
{
protected:
	TestString ()
	{}

	virtual ~TestString ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		::Nirvana::Core::Heap::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::Core::Heap::terminate ();
	}
};

typedef ::testing::Types <Char, WChar> CharTypes;

TYPED_TEST_CASE (TestString, CharTypes);

TYPED_TEST (TestString, Var)
{
	static const TypeParam test [5] = {'T', 'E', 'S', 'T', '\0'};
	StringT_var <TypeParam> var = test;
	EXPECT_EQ (test, var);
	StringT_var <TypeParam> var1 = var;
	EXPECT_EQ ((const TypeParam*)var1, (const TypeParam*)var);
	var1 [(ULong)1] = 'A';
	EXPECT_NE ((const TypeParam*)var1, (const TypeParam*)var);
	EXPECT_EQ (var1 [(ULong)1], 'A');
}

}