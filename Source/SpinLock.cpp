#include "Thread.h"
#include "BackOff.h"

namespace Nirvana {
namespace Core {

inline
void SpinLock::acquire (SpinLockNode& node) NIRVANA_NOEXCEPT
{
	node.sn_next_ = nullptr;
	SpinLockNode* predecessor = queue_.exchange (&node);
	if (predecessor) {
		// Queue was non-empty
		for (BackOff bo; true; bo ()) {
			node.sn_locked_ = 1;
			predecessor->sn_next_ = &node;
			if (!node.sn_locked_)
				break;
		}
	}
}

inline
void SpinLock::release (SpinLockNode& node) NIRVANA_NOEXCEPT
{
	if (!node.sn_next_) {
		// No known successor
		for (BackOff bo; true; bo ()) {
			SpinLockNode* pnode = &node;
			if (queue_.compare_exchange_weak (pnode, nullptr))
				return;
			if (node.sn_next_)
				break;
		}
	}
	node.sn_next_->sn_locked_ = 0;
}

void SpinLock::acquire () NIRVANA_NOEXCEPT
{
	acquire (Thread::current ());
}

void SpinLock::release () NIRVANA_NOEXCEPT
{
	release (Thread::current ());
}


}
}
