#include "GTestSys.h"
#include <Scheduler.h>
#include <gtest/gtest.h>

namespace Nirvana {
namespace Core {
namespace Test {

GTestSys::GTestSys (int argc, char* argv []) :
	StartupSys (0, nullptr)
{
	testing::InitGoogleTest (&argc, argv);
}

void GTestSys::run ()
{
	StartupSys::run ();
	ret_ = RUN_ALL_TESTS ();
	Scheduler::shutdown ();
}

}
}
}
