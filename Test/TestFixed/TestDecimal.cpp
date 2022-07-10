
#include <Nirvana/Nirvana.h>
#include <Nirvana/Decimal.h>
#include <gtest/gtest.h>

using namespace Nirvana;
using namespace std;

TEST (TestDecimal, Conversion)
{
	{
		Decimal <5, 4> d;
		EXPECT_EQ (d.to_string (), "0.0000");
	}

	{
		Decimal <5, 4> d = "1.2345";
		EXPECT_EQ (d.to_string (), "1.2345");
	}

	{
		Decimal <5, 4> d = 1;
		d = 2;
		EXPECT_THROW (d = 20, CORBA::DATA_CONVERSION);
	}
}
