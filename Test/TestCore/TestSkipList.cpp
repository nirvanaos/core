#include "../Source/SkipList.h"
#include "../Source/MemContextCore.h"
#include <gtest/gtest.h>


using namespace std;
using ::Nirvana::Core::SkipList;

namespace TestSkipList {

class TestSkipList : public ::testing::Test
{
protected:
	TestSkipList ()
	{}

	virtual ~TestSkipList ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		Nirvana::Core::MemContext::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Nirvana::Core::MemContext::terminate ();
	}
};

TEST_F (TestSkipList, Move)
{
	typedef SkipList <int, 10> SL;

	SL sl;
	sl.release_node (sl.insert (0).first);
	sl.release_node (sl.insert (1).first);
	sl.release_node (sl.insert (2).first);

	SL sl1;
	sl1 = move (sl);
	ASSERT_FALSE (sl.get_min_node ());

	int i;
	ASSERT_TRUE (sl1.delete_min (i));
	ASSERT_EQ (i, 0);
	ASSERT_TRUE (sl1.delete_min (i));
	ASSERT_EQ (i, 1);
	ASSERT_TRUE (sl1.delete_min (i));
	ASSERT_EQ (i, 2);
}

}
