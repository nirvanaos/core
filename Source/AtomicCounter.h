#ifndef NIRVANA_CORE_ATOMICCOUNTER_H_
#define NIRVANA_CORE_ATOMICCOUNTER_H_

#include "core.h"
#include <atomic>
#include <type_traits>

namespace Nirvana {
namespace Core {

template <bool SIGNED>
class AtomicCounter
{
#if ATOMIC_INT_LOCK_FREE
	typedef unsigned int Unsigned;
	typedef int Signed;
#elif ATOMIC_LONG_LOCK_FREE
	typedef unsigned long Unsigned;
	typedef long Signed;
#elif ATOMIC_LLONG_LOCK_FREE
	typedef unsigned long long Unsigned;
	typedef long long Signed;
#elif ATOMIC_SHORT_LOCK_FREE
	typedef unsigned short Unsigned;
	typedef short Signed;
#else
#error Platform does not meet the minimal atomic requirements.
#endif

public:
	using IntegralType = std::conditional_t <SIGNED, Signed, Unsigned>;

	AtomicCounter (IntegralType init) NIRVANA_NOEXCEPT :
		cnt_ (init)
	{
		assert (cnt_.is_lock_free ());
	}

	explicit operator IntegralType () const NIRVANA_NOEXCEPT
	{
		return cnt_;
	}

	operator bool () const NIRVANA_NOEXCEPT
	{
		return cnt_ != 0;
	}

	IntegralType increment () NIRVANA_NOEXCEPT
	{
		return ++cnt_;
	}

	IntegralType decrement () NIRVANA_NOEXCEPT
	{
		assert (SIGNED || cnt_ > 0);
		return --cnt_;
	}

private:
	std::atomic <IntegralType> cnt_;
};

/// Reference counter
class RefCounter : public AtomicCounter <false>
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
};

}
}

#endif
