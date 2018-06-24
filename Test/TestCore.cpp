#include "../core.h"
#include <gtest/gtest.h>

namespace unittests {

using namespace Nirvana;

class TestCore :
	public ::testing::Test
{
protected:
	TestCore ()
	{}

	virtual ~TestCore ()
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
	}
};

TEST_F (TestCore, NLZ)
{
	static const uint32_t test [] = {0,32, 1,31, 2,30, 3,30, 4,29, 5,29, 6,29,
		7,29, 8,28, 9,28, 16,27, 32,26, 64,25, 128,24, 255,24, 256,23,
		512,22, 1024,21, 2048,20, 4096,19, 8192,18, 16384,17, 32768,16,
		65536,15, 0x20000,14, 0x40000,13, 0x80000,12, 0x100000,11,
		0x200000,10, 0x400000,9, 0x800000,8, 0x1000000,7, 0x2000000,6,
		0x4000000,5, 0x8000000,4, 0x0FFFFFFF,4, 0x10000000,3,
		0x3000FFFF,2, 0x50003333,1, 0x7FFFFFFF,1, 0x80000000,0,
		0xFFFFFFFF,0};

	for (size_t i = 0; i < _countof (test); i += 2) {
		EXPECT_EQ (nlz (test [i]), test [i + 1]);
	}
}

TEST_F (TestCore, NLZ_Perf)
{
	for (uint32_t i = 10000000; i; --i) {
		EXPECT_LE (nlz (i), (uint32_t)32);
	}
}

}
