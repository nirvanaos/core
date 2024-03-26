/*
* Nirvana Core test.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include "pch.h"
#include "../Source/Stack.h"
#include <unordered_set>
#include <vector>
#include <thread>
#include <random>

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
		return _aligned_malloc (size, Nirvana::Core::core_object_align (sizeof (Value)));
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
	std::random_device rd;
	std::mt19937 rndgen (rd ());
	std::uniform_int_distribution <> distrib (0, elements);
	std::vector <Value*> buf (elements);
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
	static const unsigned thread_cnt = std::thread::hardware_concurrency ();
	static const unsigned element_cnt = 20;
	static const unsigned iterations = 10000;
	MyStack stack;
	std::unordered_set <Value*> valset;
	for (unsigned cnt = thread_cnt * element_cnt; cnt; --cnt) {
		Value* val = new Value;
		valset.insert (val);
		stack.push (*val);
	}

	std::vector <std::thread> threads;
	threads.reserve (thread_cnt);
	for (unsigned cnt = thread_cnt; cnt; --cnt) {
		threads.emplace_back (std::thread (thread_proc, std::ref (stack), element_cnt, iterations));
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