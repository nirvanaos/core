
#include <Nirvana/Nirvana.h>
#include <Nirvana/Decimal.h>
#include <gtest/gtest.h>

using namespace Nirvana;
using namespace std;
using namespace CORBA;

TEST (TestFixed, FromString)
{
	Fixed f ("1.2345");
	// Invalid string
	EXPECT_THROW (Fixed ("invalid"), DATA_CONVERSION);

	// Too many digits
	EXPECT_THROW (Fixed ("12345678901234567890123456789012345678901234567890123456789012.3"), DATA_CONVERSION);
}

