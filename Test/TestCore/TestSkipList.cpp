#include "pch.h"
#include "../Source/SkipList.h"
#include "../Source/MemContextCore.h"
#include <random>

using Nirvana::Core::SkipList;

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
		ASSERT_TRUE (Nirvana::Core::Heap::initialize ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Nirvana::Core::Heap::terminate ();
	}
};

typedef SkipList <int, 10> SL;

TEST_F (TestSkipList, Move)
{
	SL sl;
	sl.release_node (sl.insert (0).first);
	sl.release_node (sl.insert (1).first);
	sl.release_node (sl.insert (2).first);

	SL sl1;
	sl1 = std::move (sl);
	ASSERT_FALSE (sl.get_min_node ());

	int i;
	ASSERT_TRUE (sl1.delete_min (i));
	ASSERT_EQ (i, 0);
	ASSERT_TRUE (sl1.delete_min (i));
	ASSERT_EQ (i, 1);
	ASSERT_TRUE (sl1.delete_min (i));
	ASSERT_EQ (i, 2);
}

void find_and_delete (SL& sl, unsigned i)
{
	std::mt19937 rndgen (i);

	static const size_t MAX_COUNT = 1000;
	std::vector <int> rand_numbers;
	rand_numbers.reserve (MAX_COUNT);
	while (rand_numbers.size () < MAX_COUNT) {
		int r = std::uniform_int_distribution <int> (
			std::numeric_limits <int>::min (), std::numeric_limits <int>::max ())(rndgen);
		auto ins = sl.insert (r);
		sl.release_node (ins.first);
		if (ins.second)
			rand_numbers.push_back (r);
	}

	std::shuffle (rand_numbers.begin (), rand_numbers.end (), rndgen);

	for (auto n : rand_numbers) {
		SL::NodeVal* p = sl.find_and_delete (n);
		EXPECT_TRUE (p);
		sl.release_node (p);
	}
}

TEST_F (TestSkipList, FindAndDelete)
{
	SL sl;
	find_and_delete (sl, 0);
}
 
TEST_F (TestSkipList, FindAndDeleteMT)
{
	SL sl;

	const unsigned int thread_cnt = std::thread::hardware_concurrency ();
	std::vector <std::thread> threads;

	threads.reserve (thread_cnt);
	for (int it = 0; it < 10; ++it) {
		for (unsigned int i = 0; i < thread_cnt; ++i) {
			threads.emplace_back (find_and_delete, std::ref (sl), it + i + 1);
		}
		for (unsigned int i = 0; i < thread_cnt; ++i) {
			threads [i].join ();
		}
		threads.clear ();
		ASSERT_FALSE (sl.delete_min ());
	}
}

}
