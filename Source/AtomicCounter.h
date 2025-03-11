/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_ATOMICCOUNTER_H_
#define NIRVANA_CORE_ATOMICCOUNTER_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <assert.h>
#include <atomic>
#include <type_traits>

namespace Nirvana {
namespace Core {

/// Atomic counter
/// 
/// \tparam SIGNED The 'true` if counter value is signed.
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
	/// The integral type of the atomic type.
	using IntegralType = std::conditional_t <SIGNED, Signed, Unsigned>;

	AtomicCounter ()
	{}

	/// Constructor.
	/// 
	/// \param init The initial value.
	AtomicCounter (IntegralType init) noexcept :
		cnt_ (init)
	{
		assert (cnt_.is_lock_free ());
	}

	/// Get the current counter value.
	/// 
	/// Operation is performed in memory relaxed order.
	/// We can not rely on the value returned because the counter may be changed at any time.
	/// Returned value may be used only for estimation purposes or debugging, such as _refcount_value ().
	/// 
	/// \returns Integral value.
	operator IntegralType () const noexcept
	{
		return cnt_.load (std::memory_order_relaxed);
	}

	/// Get the current counter value.
	/// Guarantees the counter consistency.
	/// Operation is performed in acquire order.
	/// 
	/// \returns Integral value.
	IntegralType load () const noexcept
	{
		return cnt_.load (std::memory_order_acquire);
	}

	/// Relaxed increment.
	void increment () noexcept
	{
		cnt_.fetch_add (1, std::memory_order_relaxed);
	}

	/// Synchronized increment.
	/// Guarantees the counter consistency.
	/// 
	/// \returns The incremented value.
	IntegralType increment_seq () noexcept
	{
		return cnt_.fetch_add (1, std::memory_order_acq_rel) + 1;
	}

	/// Relaxed decrement.
	void decrement () noexcept
	{
		assert (SIGNED || cnt_ > 0);
		cnt_.fetch_sub (1, std::memory_order_relaxed);
	}

	/// Synchronized increment.
	/// Guarantees the counter consistency.
	/// 
	/// \returns The decremented value.
	IntegralType decrement_seq () noexcept
	{
		assert (SIGNED || cnt_ > 0);
		return cnt_.fetch_sub (1, std::memory_order_acq_rel) - 1;
	}

	/// Decrement counter if it is not zero.
	/// 
	/// \returns `true` if counter was decremented.
	bool decrement_if_not_zero () noexcept
	{
		IntegralType cur = cnt_.load (std::memory_order_relaxed);
		for (;;) {
			if (!cur)
				return false;
			if (cnt_.compare_exchange_weak (cur, cur - 1, std::memory_order_release, std::memory_order_relaxed))
				return true;
		}
	}

	/// Increment counter if it is not zero.
	/// 
	/// \returns `true` if counter was incremented.
	bool increment_if_not_zero () noexcept
	{
		IntegralType cur = cnt_.load (std::memory_order_relaxed);
		for (;;) {
			if (!cur)
				return false;
			if (cnt_.compare_exchange_weak (cur, cur + 1, std::memory_order_release, std::memory_order_relaxed))
				return true;
		}
	}

protected:
	std::atomic <IntegralType> cnt_;
};

/// Reference counter
///
/// - Unsigned atomic counter.
/// - Initialized to 1 on construct.
/// - Hidden in copy construction and assignments.
/// - Incrementing from 0 is prohibited.
/// 
class RefCounter : public AtomicCounter <false>
{
	typedef AtomicCounter <false> Base;
public:
	/// The integral type of the reference counter.
	using Base::IntegralType;

	/// Initialized with 1.
	RefCounter () noexcept :
		Base (1)
	{}

	/// Initialized with 1 on copy construction.
	RefCounter (const RefCounter&) noexcept :
		Base (1)
	{}

	RefCounter (RefCounter&&) noexcept :
		Base (1)
	{}

	/// Not changed on copy operation.
	RefCounter& operator = (const RefCounter&) noexcept
	{
		return *this;
	}

	RefCounter& operator = (RefCounter&&) noexcept
	{
		return *this;
	}

	/// Get the current counter value.
	/// 
	/// Operation is performed in memory relaxed order.
	/// We can not rely on the value returned because the counter may be changed at any time.
	/// Returned value may be used only for estimation purposes or debugging, such as _refcount_value ().
	/// 
	/// \returns Integral value.
	using Base::operator IntegralType;

	/// \invariant cnt_ > 0.
	///   If we have no references we can't add a new one.
	void increment () noexcept
	{
		assert (*this > 0);
		Base::increment ();
	}

	/// Decrement the counter and return the decremented value.
	/// Guarantees the memory consistency if returned 0.
	/// 
	/// \returns The decremented value.
	IntegralType decrement () noexcept
	{
		IntegralType ret = decrement_seq ();
		if (0 == ret)
			std::atomic_thread_fence (std::memory_order_acquire);
		return ret;
	}

};

}
}

#endif
