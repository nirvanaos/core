
#include <Nirvana/Nirvana.h>
#include <Nirvana/Decimal.h>
#include <gtest/gtest.h>

using namespace Nirvana;
using namespace std;
using namespace CORBA;

TEST (TestFixed, String)
{
	Fixed f = "1.2345";
	EXPECT_EQ (f.to_string (), "1.2345");

	// Invalid string
	EXPECT_THROW (Fixed ("invalid"), DATA_CONVERSION);

	// Too many digits > 62
	EXPECT_THROW (Fixed ("12345678901234567890123456789012345678901234567890123456789012.3"), DATA_CONVERSION);

}

TEST (TestFixed, RoundTruncate)
{
	Fixed f1 = "0.1";
	Fixed f2 = "0.05";
	Fixed f3 = "-0.005";

	EXPECT_EQ (f1.round (0).to_string (), "0");
	EXPECT_EQ (f1.truncate (0).to_string (), "0");
	EXPECT_EQ (f2.round (1).to_string (), "0.1");
	EXPECT_EQ (f2.truncate (1).to_string (), "0.0");
	EXPECT_EQ (f3.round (2).to_string (), "-0.01");
	EXPECT_EQ (f3.truncate (2).to_string (), "0.00");
}

