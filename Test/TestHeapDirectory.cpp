#include "../HeapDirectory.h"
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include "../core.h"

using namespace ::Nirvana;
using namespace ::std;

namespace unittests {

template <UWord SIZE, bool PROT, bool EXC>
class HeapDirectoryFactory
{
public:
	typedef HeapDirectory <SIZE, EXC> DirectoryType;

	static void initialize ()
	{
		if (PROT)
			::Nirvana::initialize ();
	}

	static void terminate ()
	{
		if (PROT)
			::Nirvana::terminate ();
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
			return prot_domain_memory ();
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
		m_directory (0)
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
		m_directory = Factory::create ();
		ASSERT_TRUE (m_directory);
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Factory::destroy (m_directory);
		Factory::terminate ();
	}

protected:
	typename DirectoryType* m_directory;
};

typedef ::testing::Types <
	HeapDirectoryFactory <0x10000, false, false>, HeapDirectoryFactory <0x10000, false, true>, HeapDirectoryFactory <0x10000, true, true>, HeapDirectoryFactory <0x10000, true, false>,
	HeapDirectoryFactory <0x8000, false, false>, HeapDirectoryFactory <0x8000, false, true>, HeapDirectoryFactory <0x8000, true, true>, HeapDirectoryFactory <0x8000, true, false>,
	HeapDirectoryFactory <0x4000, false, false>, HeapDirectoryFactory <0x4000, false, true>, HeapDirectoryFactory <0x4000, true, true>, HeapDirectoryFactory <0x4000, true, false>
> MyTypes;

TYPED_TEST_CASE (TestHeapDirectory, MyTypes);

TYPED_TEST (TestHeapDirectory, Allocate)
{
	EXPECT_TRUE (this->m_directory->empty ());

	for (int pass = 0; pass < 2; ++pass) {
		for (UWord block_size = TypeParam::DirectoryType::MAX_BLOCK_SIZE; block_size > 0; block_size >>= 1) {
			UWord blocks_cnt = TypeParam::DirectoryType::UNIT_COUNT / block_size;
			for (UWord i = 0; i < blocks_cnt; ++i) {
				EXPECT_FALSE (this->m_directory->check_allocated (i * block_size, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
				UWord block = this->m_directory->allocate (block_size, TypeParam::memory ());
				ASSERT_EQ (block, i * block_size);
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));
			}
			ASSERT_TRUE (this->m_directory->check_allocated (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
			this->m_directory->release (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ());
		}

		EXPECT_TRUE (this->m_directory->empty ());
	}
}

TYPED_TEST (TestHeapDirectory, Release)
{
	EXPECT_TRUE (this->m_directory->empty ());

	for (int pass = 0; pass < 2; ++pass) {
		for (UWord block_size = TypeParam::DirectoryType::MAX_BLOCK_SIZE; block_size >= 4; block_size >>= 1) {
			UWord blocks_cnt = TypeParam::DirectoryType::UNIT_COUNT / block_size;
			for (UWord i = 0; i < blocks_cnt; ++i) {

				// Allocate and release block.
				EXPECT_FALSE (this->m_directory->check_allocated (i * block_size, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
				UWord block = this->m_directory->allocate (block_size, TypeParam::memory ());
				ASSERT_EQ (block, i * block_size);
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));
				this->m_directory->release (block, block + block_size, TypeParam::memory ());
				EXPECT_FALSE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Allocate the same block.
				ASSERT_TRUE (this->m_directory->allocate (block, block + block_size, TypeParam::memory ()));
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Release the first half.
				this->m_directory->release (block, block + block_size / 2, TypeParam::memory ());
				EXPECT_FALSE (this->m_directory->check_allocated (block, block + block_size / 2, TypeParam::memory ()));
				EXPECT_TRUE (this->m_directory->check_allocated (block + block_size / 2, block + block_size, TypeParam::memory ()));

				// Release the second half.
				this->m_directory->release (block + block_size / 2, block + block_size, TypeParam::memory ());
				EXPECT_FALSE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Allocate the range again.
				ASSERT_TRUE (this->m_directory->allocate (block, block + block_size, TypeParam::memory ()));
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Release the second half.
				this->m_directory->release (block + block_size / 2, block + block_size, TypeParam::memory ());
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size / 2, TypeParam::memory ()));
				EXPECT_FALSE (this->m_directory->check_allocated (block + block_size / 2, block + block_size, TypeParam::memory ()));

				// Release the first half.
				this->m_directory->release (block, block + block_size / 2, TypeParam::memory ());
				EXPECT_FALSE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Allocate the range again.
				ASSERT_TRUE (this->m_directory->allocate (block, block + block_size, TypeParam::memory ()));
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Release half at center
				this->m_directory->release (block + block_size / 4, block + block_size / 4 * 3, TypeParam::memory ());
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size / 4, TypeParam::memory ()));
				EXPECT_FALSE (this->m_directory->check_allocated (block + block_size / 4, block + block_size / 4 * 3, TypeParam::memory ()));
				EXPECT_TRUE (this->m_directory->check_allocated (block + block_size / 4 * 3, block + block_size, TypeParam::memory ()));

				// Release the first quarter.
				this->m_directory->release (block, block + block_size / 4, TypeParam::memory ());
				EXPECT_FALSE (this->m_directory->check_allocated (block, block + block_size / 4 * 3, TypeParam::memory ()));
				EXPECT_TRUE (this->m_directory->check_allocated (block + block_size / 4 * 3, block + block_size, TypeParam::memory ()));

				// Release the last quarter.
				this->m_directory->release (block + block_size / 4 * 3, block + block_size, TypeParam::memory ());
				EXPECT_FALSE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));

				// Allocate the first half.
				this->m_directory->allocate (block, block + block_size / 2, TypeParam::memory ());
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size / 2, TypeParam::memory ()));
				EXPECT_FALSE (this->m_directory->check_allocated (block + block_size / 2, block + block_size, TypeParam::memory ()));

				// Allocate the second half.
				this->m_directory->allocate (block + block_size / 2, block + block_size, TypeParam::memory ());
				EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));
			}
			EXPECT_TRUE (this->m_directory->check_allocated (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
			this->m_directory->release (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ());
		}

		EXPECT_TRUE (this->m_directory->empty ());
	}
}

class RandomAllocator
{
public:
	RandomAllocator (unsigned seed = mt19937::default_seed) :
		m_rndgen (seed)
	{
		m_allocated.reserve (1024);
	}

	template <class DirType>
	void run (DirType* dir, Memory_ptr memory, int iterations);

	struct Block
	{
		ULong begin;
		ULong end;
	};

	const vector <Block>& allocated () const
	{
		return m_allocated;
	}

private:
	mt19937 m_rndgen;
	vector <Block> m_allocated;
	static atomic <ULong> sm_total_allocated;
};

atomic <ULong> RandomAllocator::sm_total_allocated = 0;

template <class DirType>
void RandomAllocator::run (DirType* dir, Memory_ptr memory, int iterations)
{
	for (int i = 0; i < iterations; ++i) {
		bool rel = !m_allocated.empty () && bernoulli_distribution ((double)sm_total_allocated / (double)DirType::UNIT_COUNT)(m_rndgen);
		if (!rel) {
			ULong free_cnt = DirType::UNIT_COUNT - sm_total_allocated;
			if (!free_cnt)
				rel = true;
			else {
				ULong max_size = DirType::MAX_BLOCK_SIZE;
				if (max_size > free_cnt)
					max_size = 0x80000000 >> nlz ((ULong)free_cnt);

				ULong size = uniform_int_distribution <ULong> (1, max_size)(m_rndgen);
				Word block = dir->allocate (size, memory);
				if (block >= 0) {
					sm_total_allocated += size;
					m_allocated.push_back ({(ULong)block, (ULong)block + size});
					EXPECT_TRUE (dir->check_allocated (m_allocated.back ().begin, m_allocated.back ().end, memory));
				} else
					rel = true;
			}
		}

		if (rel) {
			ASSERT_FALSE (m_allocated.empty ());
			size_t idx = uniform_int_distribution <size_t> (0, m_allocated.size () - 1)(m_rndgen);
			Block& block = m_allocated [idx];
			EXPECT_TRUE (dir->check_allocated (block.begin, block.end, memory));
			dir->release (block.begin, block.end, memory);
			sm_total_allocated -= block.end - block.begin;
			m_allocated.erase (m_allocated.begin () + idx);
		}
	}
}

TYPED_TEST (TestHeapDirectory, Random)
{
	EXPECT_TRUE (this->m_directory->empty ());

	RandomAllocator ra;
	static const int MAX_ITERATIONS = 1000000;
	ra.run (this->m_directory, TypeParam::memory (), MAX_ITERATIONS);

	for (auto p = ra.allocated ().cbegin (); p != ra.allocated ().cend (); ++p) {
		EXPECT_TRUE (this->m_directory->check_allocated (p->begin, p->end, TypeParam::memory ()));
		this->m_directory->release (p->begin, p->end, TypeParam::memory ());
	}

	EXPECT_TRUE (this->m_directory->empty ());
}

class ThreadAllocator :
	public RandomAllocator,
	public thread
{
public:
	template <class DirType>
	ThreadAllocator (unsigned n, DirType* dir, Memory_ptr memory, int iterations) :
		RandomAllocator (n),
		thread (&RandomAllocator::template run <DirType>, this, dir, memory, iterations)
	{}
};

TYPED_TEST (TestHeapDirectory, MultiThread)
{
	EXPECT_TRUE (this->m_directory->empty ());

	static const size_t THREAD_CNT = 2;
	static const int MAX_ITERATIONS = 1000000;
	vector <ThreadAllocator> threads;
	threads.reserve (THREAD_CNT);
	for (unsigned i = 0; i < THREAD_CNT; ++i) {
		threads.emplace_back (i, this->m_directory, TypeParam::memory (), MAX_ITERATIONS);
	}

	for (auto p = threads.begin (); p != threads.end (); ++p) {
		p->join ();
	}

	for (auto pt = threads.begin (); pt != threads.end (); ++pt) {
		for (auto p = pt->allocated ().cbegin (); p != pt->allocated ().cend (); ++p) {
			EXPECT_TRUE (this->m_directory->check_allocated (p->begin, p->end, TypeParam::memory ()));
			this->m_directory->release (p->begin, p->end, TypeParam::memory ());
		}
	}

	EXPECT_TRUE (this->m_directory->empty ());
}

}
