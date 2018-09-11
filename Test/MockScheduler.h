// Nirvana project
// Core tests
// MockScheduler class

#ifndef NIRVANA_CORE_TEST_MOCKSCHEDULER_H_
#define NIRVANA_CORE_TEST_MOCKSCHEDULER_H_

namespace Nirvana {
namespace Core {
namespace Test {

void mock_scheduler_init (bool backoff_break);
void mock_scheduler_term ();
void mock_scheduler_backoff_break (bool on);

}
}
}

#endif


