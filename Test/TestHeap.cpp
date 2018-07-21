#include "../Source/core.h"
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <atomic>
#include <nlzntz.h>

using namespace ::CORBA;
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
		::Nirvana::Core::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::Core::terminate ();
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
	static const UWord GRANULARITY = 128;
	static const UWord BLOCK_SIZE = GRANULARITY * 128;
	static const UWord COUNT = 1024 * 1024 * 4 / 16 * GRANULARITY / BLOCK_SIZE;
	void* blocks [COUNT];
	Memory_ptr heap = g_heap_factory->create_with_granularity (GRANULARITY);
	EXPECT_EQ (GRANULARITY, heap->query (0, Memory::ALLOCATION_UNIT));

	for (int i = 0; i < COUNT; ++i) {
		blocks [i] = heap->allocate (0, BLOCK_SIZE, 0);
		ASSERT_TRUE (blocks [i]);
		Word au = heap->query (blocks [i], Memory::ALLOCATION_UNIT);
		ASSERT_EQ (GRANULARITY, au);
	}
	
	for (int i = COUNT - 1; i >= 0; --i) {
		heap->release (blocks [i], BLOCK_SIZE);
	}

	release (heap);
}

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
		m_rndgen (seed)
	{
		m_allocated.reserve (1024);
	}

	void run (Memory_ptr memory, int iterations);

	const vector <Block>& allocated () const
	{
		return m_allocated;
	}

private:
	mt19937 m_rndgen;
	vector <Block> m_allocated;
	static atomic <UWord> sm_next_tag;
	static atomic <UWord> sm_total_allocated;
};

atomic <UWord> RandomAllocator::sm_total_allocated = 0;
atomic <UWord> RandomAllocator::sm_next_tag = 1;

void RandomAllocator::run (Memory_ptr memory, int iterations)
{
	static const ULong MAX_MEMORY = 0x20000000;	// 512M
	static const ULong MAX_BLOCK = 0x1000000;	// 16M
	for (int i = 0; i < iterations; ++i) {
		UWord total = sm_total_allocated;
		bool rel = !m_allocated.empty () 
			&& (total >= MAX_MEMORY || bernoulli_distribution ((double)total / (double)MAX_MEMORY)(m_rndgen));
		if (!rel) {
			ULong size = poisson_distribution <> (16)(m_rndgen) * sizeof (UWord);
			if (!size)
				size = sizeof (UWord);
			else if (size > MAX_BLOCK)
				size = MAX_BLOCK;
			try {
				UWord* block = (UWord*)memory->allocate (0, size, 0);
				UWord tag = sm_next_tag++;
				sm_total_allocated += size;
				*block = tag;
				block [size / sizeof (UWord) - 1] = tag;
				m_allocated.push_back ({tag, block, block + size / sizeof (UWord)});
			} catch (const NO_MEMORY&) {
				rel = true;
			}
		}

		if (rel) {
			ASSERT_FALSE (m_allocated.empty ());
			size_t idx = uniform_int_distribution <size_t> (0, m_allocated.size () - 1)(m_rndgen);
			Block& block = m_allocated [idx];
			ASSERT_EQ (block.tag, *block.begin);
			ASSERT_EQ (block.tag, *(block.end - 1));
			memory->release (block.begin, (block.end - block.begin) * sizeof (UWord));
			sm_total_allocated -= (block.end - block.begin) * sizeof (UWord);
			m_allocated.erase (m_allocated.begin () + idx);
		}
	}
}

class AllocatedBlocks :
	public set <Block>
{
public:
	void add (const vector <Block>& blocks);
	void check (Memory_ptr memory);
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

void AllocatedBlocks::check (Memory_ptr memory)
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
		ra.run (g_default_heap, ALLOC_ITERATIONS);

		AllocatedBlocks checker;
		ASSERT_NO_FATAL_FAILURE (checker.add (ra.allocated ()));
		ASSERT_NO_FATAL_FAILURE (checker.check (g_default_heap));
	}

	for (auto p = ra.allocated ().cbegin (); p != ra.allocated ().cend (); ++p)
		g_default_heap->release (p->begin, (p->end - p->begin) * sizeof (UWord));
}

class ThreadAllocator :
	public RandomAllocator,
	public thread
{
public:
	ThreadAllocator (unsigned seed) :
		RandomAllocator (seed)
	{}

	void run (Memory_ptr memory, int iterations)
	{
		thread t (&RandomAllocator::run, this, memory, iterations);
		swap (t);
	}
};

TEST_F (TestHeap, MultiThread)
{
	const size_t thread_cnt = max (thread::hardware_concurrency (), (unsigned)2);
	static const int ITERATIONS = 50;
	static const int THREAD_ITERATIONS = 1000;
	vector <ThreadAllocator> threads;
	threads.reserve (thread_cnt);
	for (unsigned i = 0; i < thread_cnt; ++i)
		threads.emplace_back (i + 1);

	for (int i = 0; i < ITERATIONS; ++i) {
		for (auto p = threads.begin (); p != threads.end (); ++p)
			p->run (g_default_heap, THREAD_ITERATIONS);

		for (auto p = threads.begin (); p != threads.end (); ++p)
			p->join ();

		AllocatedBlocks checker;
		for (auto pt = threads.begin (); pt != threads.end (); ++pt)
			ASSERT_NO_FATAL_FAILURE (checker.add (pt->allocated ()));

		ASSERT_NO_FATAL_FAILURE (checker.check (g_default_heap));
	}

	for (auto pt = threads.begin (); pt != threads.end (); ++pt) {
		for (auto p = pt->allocated ().cbegin (); p != pt->allocated ().cend (); ++p)
			g_default_heap->release (p->begin, (p->end - p->begin) * sizeof (UWord));
	}
}

}
