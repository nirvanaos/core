#ifndef NIRVANA_CORE_ATOMICCOUNTER_H_
#define NIRVANA_CORE_ATOMICCOUNTER_H_

#include "core.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class AtomicCounter
{
public:
	typedef UWord UIntType;
	typedef Word IntType;

	AtomicCounter (IntType init) NIRVANA_NOEXCEPT :
		cnt_ (init)
	{
		assert (cnt_.is_lock_free ());
	}

	explicit operator IntType () const NIRVANA_NOEXCEPT
	{
		return cnt_;
	}

	explicit operator UIntType () const NIRVANA_NOEXCEPT
	{
		return cnt_;
	}

	operator bool () const NIRVANA_NOEXCEPT
	{
		return cnt_ != 0;
	}

	IntType increment () NIRVANA_NOEXCEPT
	{
		return ++cnt_;
	}

	IntType decrement () NIRVANA_NOEXCEPT
	{
		return --cnt_;
	}

private:
	std::atomic <Word> cnt_;
};

/// Reference counter
class RefCounter : public AtomicCounter
{
public:
	/// Initialized with 1.
	RefCounter () NIRVANA_NOEXCEPT :
		AtomicCounter (1)
	{}

	/// Initialized with 1 on copy construction.
	RefCounter (const RefCounter&) NIRVANA_NOEXCEPT :
		AtomicCounter (1)
	{}

	/// Not changed on copy operation.
	RefCounter& operator = (const RefCounter&) NIRVANA_NOEXCEPT
	{
		return *this;
	}

	UIntType decrement () NIRVANA_NOEXCEPT
	{
		assert ((UIntType)*this > 0);
		return AtomicCounter::decrement ();
	}

	UIntType increment () NIRVANA_NOEXCEPT
	{
		return AtomicCounter::increment ();
	}
};

}
}

#endif
