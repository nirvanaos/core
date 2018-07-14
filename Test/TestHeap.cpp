#include "../core.h"
#include <gtest/gtest.h>

using namespace ::CORBA;
using namespace ::Nirvana;
using namespace ::std;

namespace unittests {

class TestHeap :
	public ::testing::Test
{
protected:
	TestHeap ()
	{}

	virtual ~TestHeap ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		::Nirvana::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::terminate ();
	}
};

TEST_F (TestHeap, Allocate)
{
	int* p = (int*)g_default_heap->allocate (0, sizeof (int), Memory::ZERO_INIT);
	*p = 1;
	g_default_heap->release (p, sizeof (int));
}

TEST_F (TestHeap, Heap)
{
	Memory_ptr heap = g_heap_factory->create ();
	int* p = (int*)heap->allocate (0, sizeof (int), Memory::ZERO_INIT);
	*p = 1;
	heap->release (p, sizeof (int));
	release (heap);
}

}
