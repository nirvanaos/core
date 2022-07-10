
#include <Nirvana/Nirvana.h>
#include <gtest/gtest.h>
#include <sstream>

using namespace Nirvana;
using namespace std;
using namespace CORBA;

TEST (TestFixed, Conversion)
{
	{
		Fixed f;
		EXPECT_EQ (f.to_string (), "0");
		EXPECT_EQ (f.fixed_digits (), 1);
		EXPECT_EQ (f.fixed_scale (), 0);
	}

	{
		Fixed f (-12345);
		EXPECT_EQ (f.to_string (), "-12345");
		EXPECT_EQ (f.fixed_digits (), 5);
		EXPECT_EQ (f.fixed_scale (), 0);
	}

	{
		Fixed f (12345U);
		EXPECT_EQ (f.to_string (), "12345");
		EXPECT_EQ (f.fixed_digits (), 5);
		EXPECT_EQ (f.fixed_scale (), 0);
	}

	{
		Fixed f (-12345LL);
		EXPECT_EQ (f.to_string (), "-12345");
		EXPECT_EQ (f.fixed_digits (), 5);
		EXPECT_EQ (f.fixed_scale (), 0);
	}

	{
		Fixed f (12345ULL);
		EXPECT_EQ (f.to_string (), "12345");
		EXPECT_EQ (f.fixed_digits (), 5);
		EXPECT_EQ (f.fixed_scale (), 0);
	}

	{
		Fixed f (1.2345);
		EXPECT_EQ (f.to_string (), "1.2345");
		EXPECT_EQ (f.fixed_digits (), 5);
		EXPECT_EQ (f.fixed_scale (), 4);
	}

	{
		Fixed f ("1.2345");
		EXPECT_EQ (f.to_string (), "1.2345");
		EXPECT_EQ (f.fixed_digits (), 5);
		EXPECT_EQ (f.fixed_scale (), 4);
	}

	// Invalid string
	EXPECT_THROW (Fixed ("invalid"), DATA_CONVERSION);

	// Too many digits > 62
	EXPECT_THROW (Fixed ("12345678901234567890123456789012345678901234567890123456789012.3"), DATA_CONVERSION);

	{
		static const IDL::FixedCDR <5, 4> cdr = { { 0x12, 0x34, 0x5C } };
		Fixed f (cdr);
		EXPECT_EQ (f.to_string (), "1.2345");
		EXPECT_EQ (f.fixed_digits (), 5);
		EXPECT_EQ (f.fixed_scale (), 4);
	}

	EXPECT_EQ (-12345LL, static_cast <long long> (Fixed (-12345)));
	EXPECT_EQ (1.5, static_cast <long double> (Fixed ("1.5")));
	EXPECT_EQ (2, static_cast <long long> (Fixed ("1.5")));
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

TEST (TestFixed, Stream)
{
	Fixed a = 1.234;
	stringstream ss;
	ss << a;
	Fixed b;
	ss >> b;
	EXPECT_FALSE (ss.fail ());
	EXPECT_EQ (a, b);
}

TEST (TestFixed, Arithmetic)
{
	EXPECT_EQ (Fixed (2) + Fixed (2), Fixed (4));
	EXPECT_EQ (Fixed (2) * Fixed (2), Fixed (4));
	EXPECT_EQ (Fixed (10) / Fixed (2), Fixed (5));

	bool ok = false;
	try {
		Fixed (10) / Fixed (0);
	} catch (const ARITHMETIC_ERROR& ex) {
		EXPECT_EQ (ex.minor (), FPE_FLTDIV);
		ok = true;
	}
	EXPECT_TRUE (ok);

	ok = false;
	try {
		Fixed (0) / Fixed (0);
	} catch (const ARITHMETIC_ERROR& ex) {
		EXPECT_EQ (ex.minor (), FPE_FLTDIV);
		ok = true;
	}
	EXPECT_TRUE (ok);
}

TEST (TestFixed, IncDec)
{
	Fixed f (1);
	EXPECT_EQ (++f, Fixed (2));
	EXPECT_EQ (f, Fixed (2));
	EXPECT_EQ (--f, Fixed (1));
	EXPECT_EQ (f, Fixed (1));
	EXPECT_EQ (f++, Fixed (1));
	EXPECT_EQ (f, Fixed (2));
	EXPECT_EQ (f--, Fixed (2));
	EXPECT_EQ (f, Fixed (1));
}

TEST (TestFixed, Compare)
{
	EXPECT_EQ (Fixed (10), Fixed (10));
	EXPECT_LE (Fixed (10), Fixed (10));
	EXPECT_GE (Fixed (10), Fixed (10));
	EXPECT_NE (Fixed (1), Fixed (2));
	EXPECT_LT (Fixed (1), Fixed (2));
	EXPECT_GT (Fixed (2), Fixed (1));

	EXPECT_TRUE (Fixed (1));
	EXPECT_FALSE (Fixed (0));
}
