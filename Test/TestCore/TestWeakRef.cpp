#include "pch.h"
#include "../Source/WeakRef.h"
#include "../Source/HeapUser.h"

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
	SmallObject (const std::string& v) :
		v_ (v)
	{}

	const std::string& v () const
	{
		return v_;
	}

private:
	std::string v_;
};

class LargeObject
{
public:
	LargeObject (const std::string& v) :
		v1_ (v),
		v2_ (v)
	{}

	const std::string& v1 () const
	{
		return v1_;
	}

	const std::string& v2 () const
	{
		return v2_;
	}

private:
	std::string v1_, v2_;
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
	strong = nullptr;
	{
		SharedRef <TypeParam> s = weak.lock ();
		EXPECT_FALSE (s);
	}
	weak = nullptr;
}

TYPED_TEST (TestWeakRef, ReleaseWeak)
{
	SharedRef <TypeParam> strong = SharedRef <TypeParam>::template create <ImplShared <TypeParam> > ("test");
	WeakRef <TypeParam> weak (strong);
	EXPECT_TRUE (weak);
	SharedRef <TypeParam> s = weak.lock ();
	EXPECT_TRUE (s);
	strong = nullptr;
	weak = nullptr;
	s = nullptr;
}

}
