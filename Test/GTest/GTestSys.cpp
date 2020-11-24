#include "GTestSys.h"
#include <gtest/gtest.h>

namespace Nirvana {
namespace Core {
namespace Test {

void GTestSys::run ()
{
	StartupSys::run ();
	testing::InitGoogleTest (&argc_, argv_);
	ret_ = RUN_ALL_TESTS ();
}

}
}
}
