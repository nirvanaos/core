/*
* Nirvana Core test.
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
#include "pch.h"
#include "../Source/FileLockRanges.h"

using Nirvana::Core::FileLockRanges;

namespace TestFileLock {

class TestFileLock :
	public ::testing::Test
{
protected:
	TestFileLock ()
	{}

	virtual ~TestFileLock ()
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

	static const void* owner (uintptr_t i)
	{
		return (const void*)i;
	}

	static const Nirvana::FileSize FILE_SIZE_MAX = std::numeric_limits <Nirvana::FileSize>::max ();
};

TEST_F (TestFileLock, Empty)
{
	FileLockRanges ranges;

	EXPECT_TRUE (ranges.check_read (0, FILE_SIZE_MAX, owner (1)));
	EXPECT_TRUE (ranges.check_write (0, FILE_SIZE_MAX, owner (1)));
}

}
