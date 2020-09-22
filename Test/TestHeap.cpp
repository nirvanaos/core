#include "../Source/core.h"
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <atomic>

using namespace ::Nirvana;
using namespace ::std;

namespace TestHeap {

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
		::Nirvana::Core::Heap::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::Core::Heap::terminate ();
	}
};

TEST_F (TestHeap, Allocate)
{
	int* p = (int*)Core::g_core_heap->allocate (0, sizeof (int), Memory::ZERO_INIT);
	*p = 1;
	Core::g_core_heap->release (p, sizeof (int));
}
/*
TEST_F (TestHeap, Heap)
{
	static const UWord GRANULARITY = 128;
	static const UWord BLOCK_SIZE = GRANULARITY * 128;
	static const UWord COUNT = 1024 * 1024 * 4 / 16 * GRANULARITY / BLOCK_SIZE;
	void* blocks [COUNT];
	Memory_ptr heap = g_heap_factory->create_with_granularity (GRANULARITY);
	EXPECT_EQ (GRANULARITY, heap->query (0, MemQuery::ALLOCATION_UNIT));

	for (int i = 0; i < COUNT; ++i) {
		blocks [i] = heap->allocate (0, BLOCK_SIZE, 0);
		ASSERT_TRUE (blocks [i]);
		Word au = heap->query (blocks [i], MemQuery::ALLOCATION_UNIT);
		ASSERT_EQ (GRANULARITY, au);
	}
	
	for (int i = COUNT - 1; i >= 0; --i) {
		heap->release (blocks [i], BLOCK_SIZE);
	}

	release (heap);
}
*/
struct Block
{
	UWord tag;
	UWord* begin;
	UWord* end;

	bool operator < (const Block& rhs) const
	{
		return begin < rhs.begin;
	}
};

class RandomAllocator
{
public:
	RandomAllocator (unsigned seed = mt19937::default_seed) :
		rndgen_ (seed)
	{
		allocated_.reserve (1024);
	}

	void run (Core::Heap* memory, int iterations);

	const vector <Block>& allocated () const
	{
		return allocated_;
	}

private:
	mt19937 rndgen_;
	vector <Block> allocated_;
	static atomic <UWord> next_tag_;
	static atomic <size_t> total_allocated_;
};

atomic <size_t> RandomAllocator::total_allocated_ = 0;
atomic <UWord> RandomAllocator::next_tag_ = 1;

void RandomAllocator::run (Core::Heap* memory, int iterations)
{
	static const size_t MAX_MEMORY = 0x20000000;	// 512M
	static const size_t MAX_BLOCK = 0x1000000;	// 16M
	for (int i = 0; i < iterations; ++i) {
		size_t total = total_allocated_;
		enum Op
		{
			OP_ALLOCATE,
			OP_COPY_RO,
			OP_COPY_RW,
			OP_COPY_CHANGE,

			OP_RELEASE
		};
		Op op;
		if (allocated_.empty ())
			op = OP_ALLOCATE;
		else if (total >= MAX_MEMORY || bernoulli_distribution ((double)total / (double)MAX_MEMORY)(rndgen_))
			op = OP_RELEASE;
		else
			op = (Op)uniform_int_distribution <> (OP_ALLOCATE, OP_RELEASE - 1) (rndgen_);

		if (op != OP_RELEASE) {
			try {
				switch (op) {
					case OP_ALLOCATE:
					{
						size_t size = uniform_int_distribution <size_t> (1, MAX_BLOCK / sizeof (UWord))(rndgen_) * sizeof (UWord);
						UWord* block = (UWord*)memory->allocate (nullptr, size, 0);
						total_allocated_ += size;
						UWord tag = next_tag_++;
						*block = tag;
						block [size / sizeof (UWord) - 1] = tag;
						allocated_.push_back ({ tag, block, block + size / sizeof (UWord) });
					}
					break;

					case OP_COPY_RO:
						break; // TODO: Does not work

					case OP_COPY_RW:
					case OP_COPY_CHANGE:
					{
						size_t idx = uniform_int_distribution <size_t> (0, allocated_.size () - 1)(rndgen_);
						Block& src = allocated_ [idx];
						size_t size = (src.end - src.begin) * sizeof (UWord);
						UWord* block = (UWord*)memory->copy (nullptr, src.begin, size, OP_COPY_RO == op ? Memory::READ_ONLY : 0);
						total_allocated_ += size;
						UWord tag;
						if (OP_COPY_CHANGE == op) {
							tag = next_tag_++;
							*block = tag;
							block [size / sizeof (UWord) - 1] = tag;
						} else
							tag = src.tag;
						allocated_.push_back ({ tag, block, block + size / sizeof (UWord) });
					}
				}
			} catch (const CORBA::NO_MEMORY&) {
				op = OP_RELEASE;
			}
		}

		if (OP_RELEASE == op) {
			ASSERT_FALSE (allocated_.empty ());
			size_t idx = uniform_int_distribution <size_t> (0, allocated_.size () - 1)(rndgen_);
			Block& block = allocated_ [idx];
			ASSERT_EQ (block.tag, *block.begin);
			ASSERT_EQ (block.tag, *(block.end - 1));
			memory->release (block.begin, (block.end - block.begin) * sizeof (UWord));
			total_allocated_ -= (block.end - block.begin) * sizeof (UWord);
			allocated_.erase (allocated_.begin () + idx);
		}
	}
}

class AllocatedBlocks :
	public set <Block>
{
public:
	void add (const vector <Block>& blocks);
	void check (Core::Heap* memory);
};

void AllocatedBlocks::add (const vector <Block>& blocks)
{
	for (auto p = blocks.cbegin (); p != blocks.cend (); ++p) {
		ASSERT_EQ (p->tag, *p->begin);
		ASSERT_EQ (p->tag, *(p->end - 1));
		auto ins = insert (*p);
		ASSERT_TRUE (ins.second);
		if (ins.first != begin ()) {
			auto prev = ins.first;
			--prev;
			ASSERT_LE (prev->end, ins.first->begin);
		}
		auto next = ins.first;
		++next;
		if (next != end ()) {
			ASSERT_LE (ins.first->end, next->begin);
		}
	}
}

void AllocatedBlocks::check (Core::Heap* memory)
{
	for (auto p = cbegin (); p != cend (); ++p) {
		memory->release (p->begin, (p->end - p->begin) * sizeof (UWord));
		UWord* bl = (UWord*)memory->allocate (p->begin, (p->end - p->begin) * sizeof (UWord), Memory::EXACTLY);
		assert (bl);
		ASSERT_EQ (p->begin, bl);
		*(p->begin) = p->tag;
		*(p->end - 1) = p->tag;
	}
}

TEST_F (TestHeap, Random)
{
	RandomAllocator ra;
	static const int ITERATIONS = 100;
	static const int ALLOC_ITERATIONS = 1000;
	for (int i = 0; i < ITERATIONS; ++i) {
		ra.run (Core::g_core_heap, ALLOC_ITERATIONS);

		AllocatedBlocks checker;
		ASSERT_NO_FATAL_FAILURE (checker.add (ra.allocated ()));
		ASSERT_NO_FATAL_FAILURE (checker.check (Core::g_core_heap));
	}

	for (auto p = ra.allocated ().cbegin (); p != ra.allocated ().cend (); ++p)
		Core::g_core_heap->release (p->begin, (p->end - p->begin) * sizeof (UWord));
}

class ThreadAllocator :
	public RandomAllocator,
	public thread
{
public:
	ThreadAllocator (unsigned seed) :
		RandomAllocator (seed)
	{}

	void run (Core::Heap* memory, int iterations)
	{
		thread t (&RandomAllocator::run, this, memory, iterations);
		swap (t);
	}
};

TEST_F (TestHeap, MultiThread)
{
	const unsigned int thread_cnt = max (thread::hardware_concurrency (), (unsigned)2);
	static const int ITERATIONS = 50;
	static const int THREAD_ITERATIONS = 1000;
	vector <ThreadAllocator> threads;
	threads.reserve (thread_cnt);
	for (unsigned int i = 0; i < thread_cnt; ++i)
		threads.emplace_back (i + 1);

	for (int i = 0; i < ITERATIONS; ++i) {
		for (auto p = threads.begin (); p != threads.end (); ++p)
			p->run (Core::g_core_heap, THREAD_ITERATIONS);

		for (auto p = threads.begin (); p != threads.end (); ++p)
			p->join ();

		AllocatedBlocks checker;
		for (auto pt = threads.begin (); pt != threads.end (); ++pt)
			ASSERT_NO_FATAL_FAILURE (checker.add (pt->allocated ()));

		ASSERT_NO_FATAL_FAILURE (checker.check (Core::g_core_heap));
	}

	for (auto pt = threads.begin (); pt != threads.end (); ++pt) {
		for (auto p = pt->allocated ().cbegin (); p != pt->allocated ().cend (); ++p)
			Core::g_core_heap->release (p->begin, (p->end - p->begin) * sizeof (UWord));
	}
}

}
