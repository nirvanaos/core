#include "../HeapDirectory.h"
#include <gtest/gtest.h>
#include <random>
#include "../core.h"

using namespace ::Nirvana;
using namespace ::std;

namespace unittests {

template <UWord SIZE, bool PROT>
class HeapDirectoryFactory
{
public:
	typedef HeapDirectory <SIZE> DirectoryType;

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
	HeapDirectoryFactory <0x10000, false>, HeapDirectoryFactory <0x8000, false>, HeapDirectoryFactory <0x4000, false>,
	HeapDirectoryFactory <0x10000, true>, HeapDirectoryFactory <0x8000, true>, HeapDirectoryFactory <0x4000, true>
> MyTypes;

TYPED_TEST_CASE (TestHeapDirectory, MyTypes);

TYPED_TEST (TestHeapDirectory, Allocate)
{
	EXPECT_TRUE (this->m_directory->empty ());

	size_t total = 0;
	for (UWord block_size = TypeParam::DirectoryType::MAX_BLOCK_SIZE; block_size > 0; block_size >>= 1) {
		UWord blocks_cnt = TypeParam::DirectoryType::UNIT_COUNT / block_size;
		for (UWord i = 0; i < blocks_cnt; ++i) {
			EXPECT_FALSE (this->m_directory->check_allocated (i * block_size, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
			UWord block = this->m_directory->allocate (block_size, TypeParam::memory ());
			ASSERT_EQ (block, i * block_size);
			EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));
			++total;
		}
		ASSERT_TRUE (this->m_directory->check_allocated (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
		this->m_directory->release (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ());
	}

	EXPECT_TRUE (this->m_directory->empty ());
}

TYPED_TEST (TestHeapDirectory, Release)
{
	EXPECT_TRUE (this->m_directory->empty ());

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

TYPED_TEST (TestHeapDirectory, Random)
{
	EXPECT_TRUE (this->m_directory->empty ());

	struct Block
	{
		UWord begin;
		UWord end;
	};

	vector <Block> allocated;
	allocated.reserve (1024);
	UWord total_allocated = 0;
	
	mt19937 rndgen;
	int iteration = 0;
	static const int MAX_ITERATIONS = 1000000;
	for (; iteration < MAX_ITERATIONS; ++iteration) {
		if (allocated.empty () || !bernoulli_distribution ((double)total_allocated / (double)TypeParam::DirectoryType::UNIT_COUNT)(rndgen)) {
			UWord size = uniform_int_distribution <UWord> (1, TypeParam::DirectoryType::MAX_BLOCK_SIZE)(rndgen);
			Word block = this->m_directory->allocate (size, TypeParam::memory ());
			if (block >= 0) {
				allocated.push_back ({(UWord)block, (UWord)block + size});
				total_allocated += size;
				EXPECT_TRUE (this->m_directory->check_allocated (allocated.back ().begin, allocated.back ().end, TypeParam::memory ()));
			} else
				break;
		} else {
			size_t idx = uniform_int_distribution <size_t> (0, allocated.size () - 1)(rndgen);
			Block& block = allocated [idx];
			EXPECT_TRUE (this->m_directory->check_allocated (block.begin, block.end, TypeParam::memory ()));
			this->m_directory->release (block.begin, block.end, TypeParam::memory ());
			total_allocated -= block.end - block.begin;
			allocated.erase (allocated.begin () + idx);
		}
	}

	for (auto p = allocated.cbegin (); p != allocated.cend (); ++p) {
		EXPECT_TRUE (this->m_directory->check_allocated (p->begin, p->end, TypeParam::memory ()));
		this->m_directory->release (p->begin, p->end, TypeParam::memory ());
	}

	EXPECT_TRUE (this->m_directory->empty ());
}

}