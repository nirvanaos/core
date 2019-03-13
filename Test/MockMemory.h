// Nirvana project
// Tests
// MockMemory class

#ifndef NIRVANA_TEST_MOCKMEMORY_H_
#define NIRVANA_TEST_MOCKMEMORY_H_

#include <Nirvana/Memory_c.h>

namespace Nirvana {
namespace Test {

Memory_ptr mock_memory ();

}

namespace Core {
namespace Test {

void mock_memory_init ();
void mock_memory_term ();

}
}

}

#endif
