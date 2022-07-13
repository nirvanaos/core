
#include <Nirvana/Nirvana.h>
#include <Nirvana/Decimal.h>
#include <gtest/gtest.h>

using namespace Nirvana;
using namespace CORBA;
using namespace std;

TEST (TestDecimal, Conversion)
{
	{
		Decimal <5, 4> d;
		EXPECT_EQ (Fixed (d).to_string (), "0.0000");
	}

	{
		Decimal <5, 4> d ("1.2345");
		EXPECT_EQ (1.2345, d);
	}

	{
		Decimal <5, 4> d (1);
		d = 2;
		EXPECT_THROW (d = 20, CORBA::DATA_CONVERSION);
	}

	{
		Decimal <6, 0> d (1);
		d = 20;
		EXPECT_EQ (20LL, d);
	}
}

TEST (TestDecimal, Stream)
{
	Decimal <8, 4> a (1.234);
	stringstream ss;
	ss << a;
	Decimal <8, 4> b;
	ss >> b;
	EXPECT_FALSE (ss.fail ());
	EXPECT_EQ (a, b);
}

TEST (TestDecimal, Arithmetic)
{
	EXPECT_EQ ((Decimal <8, 4> (2) + Decimal <8, 4> (2)), Fixed (4));
	EXPECT_EQ ((Decimal <8, 4> (2) * Decimal <8, 4> (2)), Fixed (4));
	Decimal <8, 4> x (10);
	Decimal <8, 4> y (2);
	Fixed r = x / y;
	EXPECT_EQ (5LL, static_cast <int64_t> (r));
}

TEST (TestDecimal, IncDec)
{
	Decimal <8, 4> f (1);
	EXPECT_EQ (++f, Fixed (2));
	EXPECT_EQ (f, Fixed (2));
	EXPECT_EQ (--f, Fixed (1));
	EXPECT_EQ (f, Fixed (1));
	EXPECT_EQ (f++, Fixed (1));
	EXPECT_EQ (f, Fixed (2));
	EXPECT_EQ (f--, Fixed (2));
	EXPECT_EQ (f, Fixed (1));
}

TEST (TestDecimal, Compare)
{
	EXPECT_EQ ((Decimal <8, 4> (10)), (Decimal <8, 4> (10)));
	EXPECT_EQ ((Decimal <8, 4> (10)), (Decimal <10, 5> (10)));
	EXPECT_LE ((Decimal <8, 4> (10)), (Decimal <8, 4> (10)));
	EXPECT_LE ((Decimal <8, 4> (10)), (Decimal <10, 5> (10)));
	EXPECT_GE ((Decimal <8, 4> (10)), (Decimal <8, 4> (10)));
	EXPECT_GE ((Decimal <8, 4> (10)), (Decimal <10, 5> (10)));
	EXPECT_NE ((Decimal <8, 4> (1)), (Decimal <8, 4> (2)));
	EXPECT_NE ((Decimal <8, 4> (1)), (Decimal <10, 5> (2)));
	EXPECT_LT ((Decimal <8, 4> (1)), (Decimal <8, 4> (2)));
	EXPECT_LT ((Decimal <8, 4> (1)), (Decimal <10, 5> (2)));
	EXPECT_GT ((Decimal <8, 4> (2)), (Decimal <8, 4> (1)));
	EXPECT_GT ((Decimal <8, 4> (2)), (Decimal <10, 5> (1)));

	EXPECT_TRUE (Fixed (1));
	EXPECT_FALSE (Fixed (0));
}

TEST (TestDecimal, Any)
{
	Decimal <5, 4> d (1.2345);
	Any any;
	any <<= d;
	Decimal <5, 4> d1;
	EXPECT_TRUE (any >>= d1);
	EXPECT_EQ (d, d1);
}

