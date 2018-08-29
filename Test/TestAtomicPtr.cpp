#include "../Source/AtomicPtr.h"
#include "../Source/AtomicCounter.h"
#include "gtest/gtest.h"
#include <thread>
#include <vector>

using namespace ::Nirvana::Core;
using namespace ::std;

namespace TestAtomicPtr {

template <unsigned ALIGN>
struct Align
{
	static const unsigned alignment = ALIGN;
};

template <typename Align>
class TestAtomicPtr :
	public ::testing::Test
{
protected:
	static const unsigned NUM_ITERATIONS = 10000;
	static const unsigned alignment = Align::alignment;

	struct Value
	{
		RefCounter ref_cnt;

		void add_ref ()
		{
			ref_cnt.increment ();
		}

		void remove_ref ()
		{
			if (!ref_cnt.decrement ())
				delete this;
		}

		void* operator new (size_t cb)
		{
			return _aligned_malloc (cb, alignment);
		}

		void operator delete (void* p)
		{
			_aligned_free (p);
		}
	};

	TestAtomicPtr ()
	{}

	virtual ~TestAtomicPtr ()
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
		Value* p = shared_ptr_.load ();
		if (p)
			p->remove_ref ();
	}

	typedef TaggedPtrT <Value, alignment> Ptr;
	typedef typename Ptr::Atomic SharedPtr;

	static SharedPtr shared_ptr_;

public:
	static void run ()
	{
		for (unsigned cnt = NUM_ITERATIONS; cnt; --cnt) {
			Value* pnew = new Value;
			for (;;) {
				auto old = shared_ptr_.lock ();
				Value* p = old;
				if (p)
					p->add_ref ();
				shared_ptr_.unlock ();
				if (shared_ptr_.cas (old, pnew)) {
					if (p) {
						p->remove_ref ();
						p->remove_ref ();
					}
					break;
				} else if (p)
					p->remove_ref ();
			}
		}
	}
};

template <typename Align>
typename TestAtomicPtr <Align>::SharedPtr TestAtomicPtr <Align>::shared_ptr_ = TestAtomicPtr <Align>::Ptr (nullptr);

typedef ::testing::Types <Align <4>, Align <32> > Alignments;

TYPED_TEST_CASE (TestAtomicPtr, Alignments);

TYPED_TEST (TestAtomicPtr, Run)
{
	const size_t thread_cnt = max (thread::hardware_concurrency (), (unsigned)2);
	vector <thread> threads;
	threads.reserve (thread_cnt);
	for (unsigned int i = 0; i < thread_cnt; ++i)
		threads.emplace_back (&TestAtomicPtr <TypeParam>::run);
	for (auto t = threads.begin (); t != threads.end (); ++t) {
		t->join ();
	}
}

}
