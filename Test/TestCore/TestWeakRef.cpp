#include "../Source/WeakRef.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Nirvana::Core;

namespace TestWeakRef {

template <class Obj>
class TestWeakRef :
	public ::testing::Test
{
protected:
	TestWeakRef ()
	{}

	virtual ~TestWeakRef ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		ASSERT_TRUE (Nirvana::Core::Heap::initialize ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Nirvana::Core::Heap::terminate ();
	}
};

class SmallObject
{
public:
	SmallObject (const string& v) :
		v_ (v)
	{}

	const string& v () const
	{
		return v_;
	}

private:
	string v_;
};

class LargeObject
{
public:
	LargeObject (const string& v) :
		v1_ (v),
		v2_ (v)
	{}

	const string& v1 () const
	{
		return v1_;
	}

	const string& v2 () const
	{
		return v2_;
	}

private:
	string v1_, v2_;
};

typedef ::testing::Types <SmallObject, LargeObject> Types;

TYPED_TEST_SUITE (TestWeakRef, Types);

TYPED_TEST (TestWeakRef, ReleaseStrong)
{
	SharedRef <TypeParam> strong = SharedRef <TypeParam>::template create <ImplShared <TypeParam> > ("test");
	WeakRef <TypeParam> weak (strong);
	EXPECT_TRUE (weak);
	{
		SharedRef <TypeParam> s = weak.lock ();
		EXPECT_TRUE (s);
	}
	strong.reset ();
	{
		SharedRef <TypeParam> s = weak.lock ();
		EXPECT_FALSE (s);
	}
	weak.reset ();
}

TYPED_TEST (TestWeakRef, ReleaseWeak)
{
	SharedRef <TypeParam> strong = SharedRef <TypeParam>::template create <ImplShared <TypeParam> > ("test");
	WeakRef <TypeParam> weak (strong);
	EXPECT_TRUE (weak);
	SharedRef <TypeParam> s = weak.lock ();
	EXPECT_TRUE (s);
	strong.reset ();
	weak.reset ();
	s.reset ();
}

}
