#include "../HeapDirectory.h"
#include <gtest/gtest.h>

namespace unittests {

using namespace ::Nirvana;

class HeapDirectoryFactory
{
public:
	typedef HeapDirectory DirectoryType;

	static HeapDirectory * create ()
	{
		HeapDirectory* p = (HeapDirectory*)calloc (1, sizeof (HeapDirectory));
		HeapDirectory::initialize (p);
		return p;
	}

	static void destroy (HeapDirectory *p)
	{
		free (p);
	}

	static Memory_ptr memory ()
	{
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
		m_directory = Factory::create ();
		ASSERT_TRUE (m_directory);
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Factory::destroy (m_directory);
	}

protected:
	typename DirectoryType* m_directory;
};

typedef ::testing::Types <HeapDirectoryFactory> MyTypes;
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
			EXPECT_EQ (block, i * block_size);
			EXPECT_TRUE (this->m_directory->check_allocated (block, block + block_size, TypeParam::memory ()));
			++total;
		}
		EXPECT_TRUE (this->m_directory->check_allocated (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ()));
		this->m_directory->release (0, TypeParam::DirectoryType::UNIT_COUNT, TypeParam::memory ());
	}

	EXPECT_TRUE (this->m_directory->empty ());
}

}
