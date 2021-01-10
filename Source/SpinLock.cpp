// Queued spin lock algorithm based on:
// Algorithms for Scalable Synchronization on
// Shared Memory Multiprocessors
// John M. Mellor-Crummey, Michael L. Scott
// http://www.cs.rice.edu/~johnmc/papers/tocs91.pdf

#include "Thread.h"

namespace Nirvana {
namespace Core {

using namespace std;

void SpinLock::acquire (SpinLockNode& node) NIRVANA_NOEXCEPT
{
	node.sn_next_ = nullptr;
	SpinLockNode* predecessor = last_.exchange (&node);
	if (predecessor) {
		// Queue was non-empty
		assert (!predecessor->sn_next_);
		node.sn_locked_ = 1;
		predecessor->sn_next_ = &node;
		while (node.sn_locked_)
			;
	}
}

void SpinLock::release (SpinLockNode& node) NIRVANA_NOEXCEPT
{
	if (!node.sn_next_) {
		// No known successor
		for (;;) {

			// If this node is last, clear and return
			SpinLockNode* pnode = &node;
			if (last_.compare_exchange_weak (pnode, nullptr))
				return;

			// Wait for a successor
			if (node.sn_next_)
				break;
		}
	}
	node.sn_next_->sn_locked_ = 0; // Unlock the successor
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
