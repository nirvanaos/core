#include "RWLock.h"
#include "BackOff.h"

namespace Nirvana {
namespace Core {

RWLock::RWLock () :
	writer_waits_ (false),
	read_cnt_ (0)
{
	write_lock_.clear ();
}

void RWLock::enter_read ()
{
	for (BackOff backoff; writer_waits_;)
		backoff ();
	while (1 == read_cnt_.increment () && write_lock_.test_and_set ())
		read_cnt_.decrement ();
}

void RWLock::enter_write ()
{
	for (BackOff backoff; write_lock_.test_and_set (); backoff ())
		writer_waits_ = true;
	writer_waits_ = false;
}

}
}
