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
#include <Nirvana/System.h>
#include <Nirvana/Legacy/Legacy.h>
#include <gtest/gtest.h>

// Tests for the Nirvana::Legacy POSIX subsystem

using namespace Nirvana;
using namespace Nirvana::Legacy;
using namespace CORBA;

namespace TestLegacy {

class TestLegacy :
	public ::testing::Test
{
protected:
	TestLegacy ()
	{}

	virtual ~TestLegacy ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		ASSERT_TRUE (Nirvana::g_system->is_legacy_mode ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}
};

TEST_F (TestLegacy, Mutex)
{
	Mutex::_ref_type mtx = Legacy::g_system->create_mutex ();
	ASSERT_TRUE (mtx);
	mtx->lock ();
	EXPECT_NO_THROW (mtx->unlock ());
	EXPECT_THROW (mtx->unlock (), BAD_INV_ORDER);
}

}
