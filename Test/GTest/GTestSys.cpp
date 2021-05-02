/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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
#include "GTestSys.h"
#include <Scheduler.h>
#include <Thread.h>
#include <ExecDomain.h>
#include <gtest/gtest.h>

namespace Nirvana {
namespace Core {
namespace Test {

GTestSys::GTestSys (int argc, char* argv [], char* envp []) :
	StartupSys (1, argv, envp)
{
	testing::InitGoogleTest (&argc, argv);
}

void GTestSys::run ()
{
	Thread::current ().exec_domain ()->heap_replace (g_core_heap.object ());
	StartupSys::run ();
	ret_ = RUN_ALL_TESTS ();
	Scheduler::shutdown ();
}

}
}
}
