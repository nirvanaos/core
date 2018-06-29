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

TYPED_TEST (TestHeapDirectory, Reserve)
{
	EXPECT_TRUE (this->m_directory->empty ());
	UWord block = this->m_directory->reserve (TypeParam::DirectoryType::MAX_BLOCK_SIZE);
	EXPECT_EQ (block, 0);
	this->m_directory->release (block, TypeParam::DirectoryType::MAX_BLOCK_SIZE);
}

}
