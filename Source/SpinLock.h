#ifndef NIRVANA_CORE_SPINLOCK_H_
#define NIRVANA_CORE_SPINLOCK_H_

#include "core.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class SpinLock;

class SpinLockNode
{
	SpinLockNode (const SpinLockNode&) = delete;
	SpinLockNode& operator = (const SpinLockNode&) = delete;
public:
	SpinLockNode () :
		sn_locked_ (0)
	{}

	~SpinLockNode () {}

private:
	friend class SpinLock;
	volatile Word sn_locked_;
	volatile SpinLockNode* sn_next_;
};

class SpinLock
{
	SpinLock (const SpinLock&) = delete;
	SpinLock& operator = (const SpinLock&) = delete;
public:
	SpinLock () NIRVANA_NOEXCEPT :
		queue_ (nullptr)
	{}

	void acquire () NIRVANA_NOEXCEPT;
	void release () NIRVANA_NOEXCEPT;

	void acquire (SpinLockNode& node) NIRVANA_NOEXCEPT;
	void release (SpinLockNode& node) NIRVANA_NOEXCEPT;

private:
	std::atomic <SpinLockNode*> queue_;
};

}
}

#endif