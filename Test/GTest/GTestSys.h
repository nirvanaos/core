#ifndef NIRVANA_CORE_TEST_GTEST_H_
#define NIRVANA_CORE_TEST_GTEST_H_

#include <StartupSys.h>

namespace Nirvana {
namespace Core {
namespace Test {

class GTestSys :
	public StartupSys
{
public:
	GTestSys (int argc, char* argv []);

	virtual void run ();
};

}
}
}

#endif
