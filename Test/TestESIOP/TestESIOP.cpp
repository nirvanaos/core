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
#include <gtest/gtest.h>
#include <Nirvana/Domains.h>

using namespace Nirvana;

namespace TestESIOP {

// Test ESIOP interaction between different protection domains on the same system.
class TestESIOP :
	public ::testing::Test
{
protected:
	TestESIOP ()
	{}

	virtual ~TestESIOP ()
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

TEST_F (TestESIOP, Performance)
{
	SysDomain::_ref_type sd = SysDomain::_narrow (CORBA::g_ORB->resolve_initial_references ("SysDomain"));
	ASSERT_TRUE (sd);
	for (int i = 0; i < 100; ++i) {
		sd->version ();
	}
}

TEST_F (TestESIOP, Platforms)
{
	SysDomain::_ref_type sd = SysDomain::_narrow (CORBA::g_ORB->resolve_initial_references ("SysDomain"));
	auto platforms = sd->supported_platforms ();
	ASSERT_GE (platforms.size (), 1u);
}

}
