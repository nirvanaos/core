#include "../Source/HeapDirectory.h"
#include "../Source/core.h"
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <atomic>
#include <vector>
#include <set>

using namespace ::Nirvana;
using namespace ::Nirvana::Core;
using namespace ::std;

namespace TestHeapDirectory {

template <UWord SIZE, bool PROT, bool EXC>
class HeapDirectoryFactory
{
public:
	typedef HeapDirectory <SIZE, EXC> DirectoryType;

	static void initialize ()
	{
		if (PROT)
			::Nirvana::Core::initialize ();
	}

	static void terminate ()
	{
		if (PROT)
			::Nirvana::Core::terminate ();
	}

	static DirectoryType* create ()
	{
		DirectoryType* p;
		if (PROT) {
			p = (DirectoryType*)memory ()->allocate (0, sizeof (DirectoryType), Memory::RESERVED | Memory::ZERO_INIT);
			DirectoryType::initialize (p, memory ());
		} else {
			p = (DirectoryType*)calloc (1, sizeof (DirectoryType));
			DirectoryType::initialize (p);
		}
		return p;
	}

	static void destroy (DirectoryType *p)
	{
		if (PROT)
			memory ()->release (p, sizeof (DirectoryType));
		else
			free (p);
	}

	static Memory_ptr memory ()
	{
		if (PROT)
			return g_protection_domain_memory;
		else
			return Memory_ptr::nil ();
	}
};

template <class Factory>
class TestHeapDirectory :
	public ::testing::Test
{
protected:
	typedef typename Factory::DirectoryType DirectoryType;

	TestHeapDirectory () :
		directory_ (0)
	{}

	virtual ~TestHeapDirectory ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		Factory::initialize ();
		directory_ = Factory::create ();
		ASSERT_TRUE (directory_);
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Factory::destroy (directory_);
		Factory::terminate ();
	}

protected:
	typename DirectoryType* directory_;
};

typedef ::testing::Types <
	HeapDirectoryFactory <0x10000, false, false>, HeapDirectoryFactory <0x10000, false, true>, HeapDirectoryFactory <0x10000, true, true>, HeapDirectoryFactory <0x10000, true, false>,
	HeapDirectoryFactory <0x8000, false, false>, HeapDirectoryFactory <0x8000, false, true>, HeapDirectoryFactory <0x8000, true, true>, HeapDirectoryFactory <0x8000, true, false>,
	HeapDirectoryFactory <0x4000, false, false>, HeapDirectoryFactory <0x4000, false, true>, HeapDirectoryFactory <0x4000, true, true>, HeapDirectoryFactory <0x4000, true, false>
> MyTypes;

TYPED_TEST_CASE (TestHeapDirectory, MyTypes);

TYPED_TEST (TestHeapDirectory, Allocate)
{
	EXPECT_TRUE (this->directory_->empty ());

	for (int pass = 0; pass < 2; ++pass) {
		for (UWord block_size = TypeParam::DirectoryType::MAX_BLOCK_SIZE; block_size > 0; block_size >>= 1) {
			UWord blocks_cnt = TypeParam::DirectoryType::UNIT_COUNT / block_size;
			for (UWord i = 0; i < blocks_cnt; ++i) {
				EXPECT_FALSE (this->directory_->check_allocated (i * block_size, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
				UWord block = this->directory_->allocate (block_size, TypeParam::memory ());
				ASSERT_EQ (block, i * block_size);
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));
			}
			ASSERT_TRUE (this->directory_->check_allocated (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
			this->directory_->release (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ());
		}

		EXPECT_TRUE (this->directory_->empty ());
	}
}

TYPED_TEST (TestHeapDirectory, Release)
{
	EXPECT_TRUE (this->directory_->empty ());

	for (int pass = 0; pass < 2; ++pass) {
		for (UWord block_size = TypeParam::DirectoryType::MAX_BLOCK_SIZE; block_size >= 4; block_size >>= 1) {
			UWord blocks_cnt = TypeParam::DirectoryType::UNIT_COUNT / block_size;
			for (UWord i = 0; i < blocks_cnt; ++i) {

				// Allocate and release block.
				EXPECT_FALSE (this->directory_->check_allocated (i * block_size, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
				UWord block = this->directory_->allocate (block_size, TypeParam::memory ());
				ASSERT_EQ (block, i * block_size);
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));
				this->directory_->release (block, block + block_size, TypeParam::memory ());
				EXPECT_FALSE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Allocate the same block.
				ASSERT_TRUE (this->directory_->allocate (block, block + block_size, TypeParam::memory ()));
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Release the first half.
				this->directory_->release (block, block + block_size / 2, TypeParam::memory ());
				EXPECT_FALSE (this->directory_->check_allocated (block, block + block_size / 2, TypeParam::memory ()));
				EXPECT_TRUE (this->directory_->check_allocated (block + block_size / 2, block + block_size, TypeParam::memory ()));

				// Release the second half.
				this->directory_->release (block + block_size / 2, block + block_size, TypeParam::memory ());
				EXPECT_FALSE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Allocate the range again.
				ASSERT_TRUE (this->directory_->allocate (block, block + block_size, TypeParam::memory ()));
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Release the second half.
				this->directory_->release (block + block_size / 2, block + block_size, TypeParam::memory ());
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size / 2, TypeParam::memory ()));
				EXPECT_FALSE (this->directory_->check_allocated (block + block_size / 2, block + block_size, TypeParam::memory ()));

				// Release the first half.
				this->directory_->release (block, block + block_size / 2, TypeParam::memory ());
				EXPECT_FALSE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Allocate the range again.
				ASSERT_TRUE (this->directory_->allocate (block, block + block_size, TypeParam::memory ()));
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Release half at center
				this->directory_->release (block + block_size / 4, block + block_size / 4 * 3, TypeParam::memory ());
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size / 4, TypeParam::memory ()));
				EXPECT_FALSE (this->directory_->check_allocated (block + block_size / 4, block + block_size / 4 * 3, TypeParam::memory ()));
				EXPECT_TRUE (this->directory_->check_allocated (block + block_size / 4 * 3, block + block_size, TypeParam::memory ()));

				// Release the first quarter.
				this->directory_->release (block, block + block_size / 4, TypeParam::memory ());
				EXPECT_FALSE (this->directory_->check_allocated (block, block + block_size / 4 * 3, TypeParam::memory ()));
				EXPECT_TRUE (this->directory_->check_allocated (block + block_size / 4 * 3, block + block_size, TypeParam::memory ()));

				// Release the last quarter.
				this->directory_->release (block + block_size / 4 * 3, block + block_size, TypeParam::memory ());
				EXPECT_FALSE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Allocate the first half.
				this->directory_->allocate (block, block + block_size / 2, TypeParam::memory ());
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size / 2, TypeParam::memory ()));
				EXPECT_FALSE (this->directory_->check_allocated (block + block_size / 2, block + block_size, TypeParam::memory ()));

				// Allocate the second half.
				this->directory_->allocate (block + block_size / 2, block + block_size, TypeParam::memory ());
				EXPECT_TRUE (this->directory_->check_allocated (block, block + block_size, TypeParam::memory ()));
			}
			EXPECT_TRUE (this->directory_->check_allocated (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
			this->directory_->release (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ());
		}

		EXPECT_TRUE (this->directory_->empty ());
	}
}

TYPED_TEST (TestHeapDirectory, Release2)
{
	EXPECT_TRUE (this->directory_->empty ());
	for (int pass = 0; pass < 2; ++pass) {

		for (UWord block = 0, end = TypeParam::DirectoryType::UNIT_COUNT; block < end; block += TypeParam::DirectoryType::MAX_BLOCK_SIZE) {
			ASSERT_TRUE (this->directory_->allocate (block, block + TypeParam::DirectoryType::MAX_BLOCK_SIZE, TypeParam::memory ()));
		}

		for (UWord block = 0, end = TypeParam::DirectoryType::UNIT_COUNT; block < end; ++block) {
			ASSERT_TRUE (this->directory_->check_allocated (block, block + 1, TypeParam::memory ()));
			this->directory_->release (block, block + 1, TypeParam::memory ());
		}

		EXPECT_TRUE (this->directory_->empty ());
	}
}

struct Block
{
	ULong begin;
	ULong end;

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

	template <class DirType>
	void run (DirType* dir, Memory_ptr memory, int iterations);

	const vector <Block>& allocated () const
	{
		return allocated_;
	}

private:
	mt19937 rndgen_;
	vector <Block> allocated_;
	static atomic <ULong> total_allocated_;
};

atomic <ULong> RandomAllocator::total_allocated_ = 0;

template <class DirType>
void RandomAllocator::run (DirType* dir, Memory_ptr memory, int iterations)
{
	for (int i = 0; i < iterations; ++i) {
		ULong total = total_allocated_;
		bool rel = !allocated_.empty () 
			&& (total >= DirType::UNIT_COUNT || bernoulli_distribution ((double)total_allocated_ / (double)DirType::UNIT_COUNT)(rndgen_));
		if (!rel) {
			ULong free_cnt = DirType::UNIT_COUNT - total_allocated_;
			if (!free_cnt)
				rel = true;
			else {
				ULong max_size = DirType::MAX_BLOCK_SIZE;
				if (max_size > free_cnt)
					max_size = 0x80000000 >> nlz ((ULong)free_cnt);

				ULong size = uniform_int_distribution <ULong> (1, max_size)(rndgen_);
				Word block = dir->allocate (size, memory);
				if (block >= 0) {
					total_allocated_ += size;
					allocated_.push_back ({(ULong)block, (ULong)block + size});
					EXPECT_TRUE (dir->check_allocated (allocated_.back ().begin, allocated_.back ().end, memory));
				} else
					rel = true;
			}
		}

		if (rel) {
			ASSERT_FALSE (allocated_.empty ());
			size_t idx = uniform_int_distribution <size_t> (0, allocated_.size () - 1)(rndgen_);
			Block& block = allocated_ [idx];
			EXPECT_TRUE (dir->check_allocated (block.begin, block.end, memory));
			dir->release (block.begin, block.end, memory);
			total_allocated_ -= block.end - block.begin;
			allocated_.erase (allocated_.begin () + idx);
		}
	}
}

class AllocatedBlocks :
	public set <Block>
{
public:
	void add (const vector <Block>& blocks);
	
	template <class DirType>
	void check (DirType* dir, Memory_ptr memory) const;
};

void AllocatedBlocks::add (const vector <Block>& blocks)
{
	for (auto p = blocks.cbegin (); p != blocks.cend (); ++p) {
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

template <class DirType>
void AllocatedBlocks::check (DirType* dir, Memory_ptr memory) const
{
	UWord free_begin = 0;
	for (auto pa = begin ();;) {
		UWord free_end;
		if (pa != end ())
			free_end = pa->begin;
		else
			free_end = DirType::UNIT_COUNT;
		if (free_begin != free_end) {
			bool ok = dir->allocate (free_begin, free_end, memory);
			ASSERT_TRUE (ok);
			dir->release (free_begin, free_end, memory);
		}
		if (pa != this->end ()) {
			ASSERT_TRUE (dir->check_allocated (pa->begin, pa->end, memory));
			free_begin = pa->end;
			++pa;
		} else
			break;
	}
}

TYPED_TEST (TestHeapDirectory, Random)
{
	EXPECT_TRUE (this->directory_->empty ());

	RandomAllocator ra;
	static const int ITERATIONS = 1000;
	static const int ALLOC_ITERATIONS = 1000;
	for (int i = 0; i < ITERATIONS; ++i) {
		ra.run (this->directory_, TypeParam::memory (), ALLOC_ITERATIONS);

		AllocatedBlocks checker;
		ASSERT_NO_FATAL_FAILURE (checker.add (ra.allocated ()));
		ASSERT_NO_FATAL_FAILURE (checker.check (this->directory_, TypeParam::memory ()));
	}
	for (auto p = ra.allocated ().cbegin (); p != ra.allocated ().cend (); ++p)
		this->directory_->release (p->begin, p->end, TypeParam::memory ());

	EXPECT_TRUE (this->directory_->empty ());
}

class ThreadAllocator :
	public RandomAllocator,
	public thread
{
public:
	ThreadAllocator (unsigned seed) :
		RandomAllocator (seed)
	{}

	template <class DirType>
	void run (DirType* dir, Memory_ptr memory, int iterations)
	{
		thread t (&RandomAllocator::template run <DirType>, this, dir, memory, iterations);
		swap (t);
	}
};

TYPED_TEST (TestHeapDirectory, MultiThread)
{
	EXPECT_TRUE (this->directory_->empty ());

	const size_t thread_cnt = max (thread::hardware_concurrency (), (unsigned)2);
	static const int THREAD_ITERATIONS = 1000;
	static const int ITERATIONS = 1000;
	vector <ThreadAllocator> threads;
	threads.reserve (thread_cnt);
	for (unsigned i = 0; i < thread_cnt; ++i)
		threads.emplace_back (i + 1);

	for (int i = 0; i < ITERATIONS; ++i) {
		for (auto p = threads.begin (); p != threads.end (); ++p)
			p->run (this->directory_, TypeParam::memory (), THREAD_ITERATIONS);

		for (auto p = threads.begin (); p != threads.end (); ++p)
			p->join ();

		AllocatedBlocks checker;
		for (auto pt = threads.begin (); pt != threads.end (); ++pt)
			ASSERT_NO_FATAL_FAILURE (checker.add (pt->allocated ()));

		ASSERT_NO_FATAL_FAILURE (checker.check (this->directory_, TypeParam::memory ()));
	}

	for (auto pt = threads.begin (); pt != threads.end (); ++pt) {
		for (auto p = pt->allocated ().cbegin (); p != pt->allocated ().cend (); ++p)
			this->directory_->release (p->begin, p->end, TypeParam::memory ());
	}

	EXPECT_TRUE (this->directory_->empty ());
}

}
