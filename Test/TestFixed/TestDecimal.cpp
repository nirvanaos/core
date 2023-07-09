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
#include <Nirvana/Decimal.h>
#include <gtest/gtest.h>

// Tests for Nirvana::Decimal class

using namespace Nirvana;
using namespace CORBA;

TEST (TestDecimal, Conversion)
{
	{
		Decimal <5, 4> d;
		EXPECT_EQ (Fixed (d).to_string (), "0.0000");
	}

	{
		Decimal <5, 4> d ("1.2345");
		EXPECT_EQ (Fixed (1.2345), d);
	}

	{
		Decimal <5, 4> d (1);
		d = Decimal <5, 4> (2);
		EXPECT_THROW ((d = Decimal <5, 4> (20)), CORBA::DATA_CONVERSION);
	}

	{
		Decimal <6, 0> d (1);
		d = Decimal <6, 0> (20);
		EXPECT_EQ (Fixed (20LL), d);
	}
}

TEST (TestDecimal, Stream)
{
	Decimal <8, 4> a (1.234);
	std::stringstream ss;
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

