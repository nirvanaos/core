#include "../Source/Heap.h"
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <atomic>
#include <signal.h>

using namespace ::Nirvana;
using namespace ::std;

namespace TestHeap {

class TestHeap :
	public ::testing::Test
{
protected:
	TestHeap ()
	{
		Core::Heap::initialize ();
	}

	virtual ~TestHeap ()
	{
		Core::Heap::terminate ();
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		EXPECT_TRUE (heap_.cleanup ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		EXPECT_TRUE (heap_.cleanup ());
	}

protected:
	Core::HeapUser heap_;
};

bool check_readable (const size_t* begin, const size_t* end, size_t tag)
{
	for (const size_t* p = begin; p != end; ++p) {
		if (*p != tag)
			return false;
	}
	return true;
}

inline bool check_readable (const void* p, size_t cb, size_t tag)
{
	return check_readable ((const size_t*)p, (const size_t*)p + cb / sizeof (size_t), tag);
}

bool check_writeable (void* p, size_t cb, size_t tag)
{
	for (size_t* pi = (size_t*)p, *end = pi + cb / sizeof (size_t); pi != end; ++pi) {
		size_t i = *pi;
		*pi = 0;
		*pi = i;
		if (i != tag)
			return false;
	}
	return true;
}

TEST_F (TestHeap, Allocate)
{
	int* p = (int*)heap_.allocate (nullptr, sizeof (int), Memory::ZERO_INIT);
	EXPECT_EQ (*p, 0);
	*p = 1;
	heap_.release (p, sizeof (int));
	EXPECT_THROW (heap_.release (p, sizeof (int)), CORBA::FREE_MEM);
}

TEST_F (TestHeap, ReadOnly)
{
	size_t pu = (size_t)heap_.query (nullptr, Memory::QueryParam::PROTECTION_UNIT);
	size_t* p = (size_t*)heap_.allocate (nullptr, pu, 0);
	size_t au = (size_t)heap_.query (p, Memory::QueryParam::ALLOCATION_UNIT);
	if (au < pu) {
		fill_n (p, pu / sizeof (size_t), 1);
		size_t* pro = (size_t*)heap_.copy (nullptr, p, pu, Memory::READ_ONLY);
		EXPECT_TRUE (check_readable (pro, pu, 1));
		size_t pu2 = (size_t)pu / 2;
		size_t* p1 = pro + pu2 / sizeof (size_t);
		heap_.release (p1, pu2);
		size_t* p2 = (size_t*)heap_.allocate (p1, pu2, 0);
		EXPECT_EQ (p1, p2);
		fill_n (p2, pu2 / sizeof (size_t), 1);
		heap_.release (pro, pu);
	}
	heap_.release (p, pu);
}

struct Block
{
	size_t tag;
	size_t* begin;
	size_t* end;

	enum
	{
		READ_WRITE,
		READ_ONLY,
		RESERVED
	}
	state;

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

	void run (Core::Heap& memory, int iterations);

	const vector <Block>& allocated () const
	{
		return allocated_;
	}

private:
	mt19937 rndgen_;
	vector <Block> allocated_;
	static atomic <size_t> next_tag_;
	static atomic <size_t> total_allocated_;
};

atomic <size_t> RandomAllocator::next_tag_ (1);
atomic <size_t> RandomAllocator::total_allocated_ (0);

void RandomAllocator::run (Core::Heap& memory, int iterations)
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
			OP_CHANGE_OR_CHECK,

			OP_RELEASE
		};
		Op op;
		if (allocated_.empty ())
			op = OP_ALLOCATE;
		else if (total >= MAX_MEMORY || bernoulli_distribution ((double)total / (double)MAX_MEMORY)(rndgen_))
			op = OP_RELEASE;
		else
			op = (Op)uniform_int_distribution <> (OP_ALLOCATE, OP_RELEASE) (rndgen_);

		if (op != OP_RELEASE) {
			try {
				switch (op) {
					case OP_ALLOCATE:
					{
						size_t size = uniform_int_distribution <size_t> (1, MAX_BLOCK / sizeof (size_t))(rndgen_) * sizeof (size_t);
						size_t* block = (size_t*)memory.allocate (nullptr, size, Memory::ZERO_INIT);
						EXPECT_TRUE (check_readable (block, size, 0)); // Check for ZERO_INIT
						total_allocated_ += size;
						size_t tag = next_tag_++;
						fill_n (block, size / sizeof (size_t), tag);
						allocated_.push_back ({ tag, block, block + size / sizeof (size_t), Block::READ_WRITE });
					}
					break;

					case OP_COPY_RO:
					case OP_COPY_RW:
					{
						size_t idx = uniform_int_distribution <size_t> (0, allocated_.size () - 1)(rndgen_);
						Block& src = allocated_ [idx];
						size_t size = (src.end - src.begin) * sizeof (size_t);
						bool read_only = OP_COPY_RO == op;
						size_t* block = (size_t*)memory.copy (nullptr, src.begin, size, read_only ? Memory::READ_ONLY : 0);
						total_allocated_ += size;
						allocated_.push_back ({ src.tag, block, block + size / sizeof (size_t), read_only ? Block::READ_ONLY : Block::READ_WRITE });
					}
					break;

					case OP_CHANGE_OR_CHECK:
					{
						size_t idx = uniform_int_distribution <size_t> (0, allocated_.size () - 1)(rndgen_);
						Block& block = allocated_ [idx];
						if (Block::RESERVED != block.state) {
							EXPECT_TRUE (check_readable (block.begin, block.end, block.tag));
							if (Block::READ_WRITE == block.state) {
								size_t tag = next_tag_++;
								fill (block.begin, block.end, tag);
								block.tag = tag;
							}
						}
					}
					break;
				}
			} catch (const CORBA::NO_MEMORY&) {
				op = OP_RELEASE;
			}
		}

		if (OP_RELEASE == op) {
			ASSERT_FALSE (allocated_.empty ());
			size_t idx = uniform_int_distribution <size_t> (0, allocated_.size () - 1)(rndgen_);
			Block& block = allocated_ [idx];
			if (Block::RESERVED != block.state)
				EXPECT_TRUE (check_readable (block.begin, block.end, block.tag));
			memory.release (block.begin, (block.end - block.begin) * sizeof (size_t));
			total_allocated_ -= (block.end - block.begin) * sizeof (size_t);
			allocated_.erase (allocated_.begin () + idx);
		}
	}
}

class AllocatedBlocks :
	public set <Block>
{
public:
	void add (const vector <Block>& blocks);
	void check (Core::Heap& memory);
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

void AllocatedBlocks::check (Core::Heap& memory)
{
	for (auto p = cbegin (); p != cend (); ++p) {
		size_t size = (p->end - p->begin) * sizeof (size_t);
		memory.release (p->begin, size);
		size_t* bl = (size_t*)memory.allocate (p->begin, size, Memory::EXACTLY | ((Block::RESERVED == p->state) ? Memory::RESERVED : 0));
		assert (bl);
		ASSERT_EQ (p->begin, bl);
		fill (p->begin, p->end, p->tag);
		if (Block::READ_ONLY == p->state)
			ASSERT_EQ (memory.copy (p->begin, p->begin, size, Memory::EXACTLY | Memory::READ_ONLY), p->begin);
	}
}

TEST_F (TestHeap, Random)
{
	RandomAllocator ra;
	static const int ITERATIONS
#ifdef _DEBUG
		= 10;
#else
		= 50;
#endif
	static const int ALLOC_ITERATIONS = 1000;
	for (int i = 0; i < ITERATIONS; ++i) {
		ASSERT_NO_FATAL_FAILURE (ra.run (heap_, ALLOC_ITERATIONS));

		AllocatedBlocks checker;
		ASSERT_NO_FATAL_FAILURE (checker.add (ra.allocated ()));
		ASSERT_NO_FATAL_FAILURE (checker.check (heap_));
	}

	for (auto p = ra.allocated ().cbegin (); p != ra.allocated ().cend (); ++p)
		heap_.release (p->begin, (p->end - p->begin) * sizeof (size_t));
}

class ThreadAllocator :
	public RandomAllocator,
	public thread
{
public:
	ThreadAllocator (unsigned seed) :
		RandomAllocator (seed)
	{}

	void run (Core::Heap& memory, int iterations)
	{
		thread t (&RandomAllocator::run, this, ref (memory), iterations);
		swap (t);
	}
};

TEST_F (TestHeap, MultiThread)
{
	const unsigned int thread_cnt = max (thread::hardware_concurrency (), (unsigned)2);
	static const int ITERATIONS
#ifdef _DEBUG
		= 5;
#else
		= 25;
#endif
	static const int THREAD_ITERATIONS = 1000;
	vector <ThreadAllocator> threads;
	threads.reserve (thread_cnt);
	for (unsigned int i = 0; i < thread_cnt; ++i)
		threads.emplace_back (i + 1);

	for (int i = 0; i < ITERATIONS; ++i) {
		for (auto p = threads.begin (); p != threads.end (); ++p)
			p->run (heap_, THREAD_ITERATIONS);

		for (auto p = threads.begin (); p != threads.end (); ++p)
			p->join ();

		AllocatedBlocks checker;
		for (auto pt = threads.begin (); pt != threads.end (); ++pt)
			ASSERT_NO_FATAL_FAILURE (checker.add (pt->allocated ()));

		ASSERT_NO_FATAL_FAILURE (checker.check (heap_));
	}

	for (auto pt = threads.begin (); pt != threads.end (); ++pt) {
		for (auto p = pt->allocated ().cbegin (); p != pt->allocated ().cend (); ++p)
			heap_.release (p->begin, (p->end - p->begin) * sizeof (size_t));
	}
}

void write_copy (Core::Heap& memory, void* src, void* dst, size_t size, int iterations)
{
	for (int cnt = iterations + 1; cnt > 0; --cnt) {
		unsigned i = 0;
		for (int* p = (int*)src, *end = p + size / sizeof (int); p != end; ++p) {
			*p = i++;
		}

		ASSERT_EQ (memory.copy (dst, src, size, 0), dst);
		i = 0;
		for (const int* p = (const int*)dst, *end = p + size / sizeof (int); p != end; ++p) {
			ASSERT_EQ (*p, i);
			++i;
		}
		i = 0;
		for (const int* p = (const int*)src, *end = p + size / sizeof (int); p != end; ++p) {
			ASSERT_EQ (*p, i);
			++i;
		}
	}
}

// Test multithread memory sharing. Exception handler must be called during this test.
TEST_F (TestHeap, MultiThreadCopy)
{
	const unsigned thread_count = max (thread::hardware_concurrency (), (unsigned)2);
	const int iterations = 100;

	size_t block_size = (size_t)heap_.query (nullptr, Memory::QueryParam::SHARING_ASSOCIATIVITY);
	uint8_t* src = (uint8_t*)heap_.allocate (nullptr, block_size, 0);
	uint8_t* dst = (uint8_t*)heap_.allocate (nullptr, block_size * thread_count, Memory::RESERVED);
	size_t thr_size = block_size / thread_count;
	vector <thread> threads;
	threads.reserve (thread_count);

	for (unsigned i = 0; i < thread_count; ++i) {
		uint8_t* ts = src + thr_size * i;
		uint8_t* td = dst + (block_size + thr_size) * i;
		threads.push_back (thread (write_copy, ref (heap_), ts, td, thr_size, iterations));
	}

	for (auto p = threads.begin (); p != threads.end (); ++p)
		p->join ();
/*
	for (size_t i = 0; i < thread_count; ++i) {
		uint8_t* ts = src + thr_size * i;
		uint8_t* td = dst + (block_size + thr_size) * i;
		write_copy (heap_, ts, td, thr_size, iterations);
	}
*/
	heap_.release (src, block_size);
	heap_.release (dst, block_size * thread_count);
}

}
