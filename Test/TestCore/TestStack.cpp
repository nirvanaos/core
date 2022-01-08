#include "../Source/Stack.h"
#include "gtest/gtest.h"
#include <unordered_set>
#include <vector>
#include <thread>
#include <random>

using namespace std;

namespace TestStack {

class TestStack :
	public ::testing::Test
{
protected:
	TestStack ()
	{}

	virtual ~TestStack ()
	{}

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

class Value : 
	public Nirvana::Core::StackElem
{
public:
	virtual ~Value ()	// Make class virtual to test unaligned StackElem
	{}

	void* operator new (size_t size)
	{
		return _aligned_malloc (size, CORE_OBJECT_ALIGN (Value));
	}

	void operator delete (void* p)
	{
		_aligned_free (p);
	}
};

typedef Nirvana::Core::Stack <Value> MyStack;

TEST_F (TestStack, SingleThread)
{
	MyStack stack;
	ASSERT_FALSE (stack.pop ());
	for (int i = 0; i < 3; ++i) {
		stack.push (*new Value);
	}
	for (int i = 0; i < 3; ++i) {
		Value* p = stack.pop ();
		ASSERT_TRUE (p);
		delete p;
	}
	ASSERT_FALSE (stack.pop ());
}

void thread_proc (MyStack& stack, unsigned elements, unsigned iterations)
{
	random_device rd;
	mt19937 rndgen (rd ());
	uniform_int_distribution <> distrib (0, elements);
	vector <Value*> buf (elements);
	while (iterations--) {
		unsigned cnt = distrib (rndgen);
		for (unsigned i = 0; i < cnt; ++i) {
			Value* val = stack.pop ();
			EXPECT_TRUE (val);
			if (!val)
				return;
			buf [i] = val;
		}
		for (unsigned i = 0; i < cnt; ++i) {
			stack.push (*buf [i]);
		}
	}
}

TEST_F (TestStack, MultiThread)
{
	static const unsigned thread_cnt = thread::hardware_concurrency ();
	static const unsigned element_cnt = 20;
	static const unsigned iterations = 10000;
	MyStack stack;
	unordered_set <Value*> valset;
	for (unsigned cnt = thread_cnt * element_cnt; cnt; --cnt) {
		Value* val = new Value;
		valset.insert (val);
		stack.push (*val);
	}

	vector <thread> threads;
	threads.reserve (thread_cnt);
	for (unsigned cnt = thread_cnt; cnt; --cnt) {
		threads.emplace_back (thread (thread_proc, ref (stack), element_cnt, iterations));
	}

	for (auto p = threads.begin (); p != threads.end (); ++p) {
		p->join ();
	}

	for (unsigned cnt = thread_cnt * element_cnt; cnt; --cnt) {
		Value* val = stack.pop ();
		ASSERT_TRUE (val);
		ASSERT_EQ (valset.erase (val), 1);
		delete val;
	}

	ASSERT_TRUE (valset.empty ());
	ASSERT_FALSE (stack.pop ());
}

}