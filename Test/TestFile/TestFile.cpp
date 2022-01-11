#include <Nirvana/Nirvana.h>
#include <Nirvana/fnctl.h>
#include <Nirvana/file_system.h>
#include <gtest/gtest.h>

using namespace Nirvana;
using namespace std;

namespace TestFile {

class TestFile :
	public ::testing::Test
{
protected:
	TestFile ()
	{
	}

	virtual ~TestFile ()
	{
	}

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
	}
};

// This test fails due to error in the heap management
TEST_F (TestFile, DISABLED_Open)
{
	char file_name [L_tmpnam_s];
	ASSERT_FALSE (tmpnam_s (file_name));
	FileAccessDirect::_ref_type fa = Static <file_factory>::ptr ()->open (file_name, O_CREAT | O_TRUNC);

	EXPECT_EQ (fa->size (), 0);
	vector <uint8_t> wbuf;
	wbuf.resize (1, 1);
	fa->write (0, wbuf);
	fa->flush ();
	vector <uint8_t> rbuf;
	fa->read (0, 1, rbuf);
	EXPECT_EQ (rbuf, wbuf);
	fa = nullptr;
}

}
