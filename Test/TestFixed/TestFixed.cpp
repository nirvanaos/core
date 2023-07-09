/*
* Nirvana test suite.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include <Nirvana/Nirvana.h>
#include <Nirvana/Fixed.h>
#include <signal.h>
#include <gtest/gtest.h>
#include <sstream>

// Tests for Nirvana::Fixed class.

using namespace Nirvana;
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
	EXPECT_THROW (Fixed ("invalid"), CORBA::DATA_CONVERSION);

	// Too many digits > 62
	EXPECT_THROW (Fixed ("12345678901234567890123456789012345678901234567890123456789012.3"), CORBA::DATA_CONVERSION);

	{
		static const FixedBCD <5, 4> cdr = { 0x12, 0x34, 0x5C };
		Fixed f (cdr);
		EXPECT_EQ (f.to_string (), "1.2345");
		EXPECT_EQ (f.fixed_digits (), 5);
		EXPECT_EQ (f.fixed_scale (), 4);
	}

	{
		static const FixedBCD <1, -3> cdr = { 0x5C };
		Fixed f (cdr);
		EXPECT_EQ (f.to_string (), "5000");
		EXPECT_EQ (f.fixed_digits (), 4);
		EXPECT_EQ (f.fixed_scale (), 0);
	}

	EXPECT_EQ (-12345LL, static_cast <long long> (Fixed (-12345)));
	EXPECT_EQ (1.5, static_cast <long double> (Fixed ("1.5")));
	EXPECT_EQ (2, static_cast <long long> (Fixed ("1.5")));
}

TEST (TestFixed, RoundTruncate)
{
	Fixed f1 ("0.1");
	Fixed f2 ("0.05");
	Fixed f3 ("-0.005");

	EXPECT_EQ (f1.round (0).to_string (), "0");
	EXPECT_EQ (f1.truncate (0).to_string (), "0");
	EXPECT_EQ (f2.round (1).to_string (), "0.1");
	EXPECT_EQ (f2.truncate (1).to_string (), "0.0");
	EXPECT_EQ (f3.round (2).to_string (), "-0.01");
	EXPECT_EQ (f3.truncate (2).to_string (), "0.00");
}

TEST (TestFixed, Stream)
{
	Fixed a (1.234);
	std::stringstream ss;
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

TEST (TestFixed, Any)
{
	Fixed f (1.2345);
	Any any;
	any <<= Any::from_fixed (f, 5, 4);
	Fixed f1;
	EXPECT_TRUE (any >>= Any::to_fixed (f1, 5, 4));
	EXPECT_EQ (f, f1);
	EXPECT_EQ (f1.fixed_digits (), 5);
	EXPECT_EQ (f1.fixed_scale (), 4);
	EXPECT_TRUE (any >>= Any::to_fixed (f1, 8, 5));
	EXPECT_EQ (f, f1);
	EXPECT_EQ (f1.fixed_digits (), 5);
	EXPECT_EQ (f1.fixed_scale (), 4);
}
