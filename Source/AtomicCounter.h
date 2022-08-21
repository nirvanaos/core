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

	/// Constructor.
	/// 
	/// \param init The initial value.
	AtomicCounter (IntegralType init) NIRVANA_NOEXCEPT :
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
	operator IntegralType () const NIRVANA_NOEXCEPT
	{
		return cnt_.load (std::memory_order_relaxed);
	}

	/// Relaxed increment.
	void increment () NIRVANA_NOEXCEPT
	{
		cnt_.fetch_add (1, std::memory_order_relaxed);
	}

	/// Synchronized increment.
	/// Guarantees the counter consistency.
	/// 
	/// \returns The incremented value.
	IntegralType increment_seq () NIRVANA_NOEXCEPT
	{
		return cnt_.fetch_add (1, std::memory_order_release) + 1;
	}

	/// Strong increment.
	/// Guarantees the memory consistency if 1 is returned.
	/// 
	/// \returns The incremented value.
	IntegralType increment_fence1 () NIRVANA_NOEXCEPT
	{
		IntegralType ret = increment_seq ();
		if (1 == ret)
			std::atomic_thread_fence (std::memory_order_acquire);
		return ret;
	}

	/// Relaxed decrement.
	void decrement () NIRVANA_NOEXCEPT
	{
		assert (SIGNED || cnt_ > 0);
		cnt_.fetch_sub (1, std::memory_order_relaxed);
	}

	/// Synchronized increment.
	/// Guarantees the counter consistency.
	/// 
	/// \returns The decremented value.
	IntegralType decrement_seq () NIRVANA_NOEXCEPT
	{
		assert (SIGNED || cnt_ > 0);
		return cnt_.fetch_sub (1, std::memory_order_release) - 1;
	}

	/// Strong decrement.
	/// Guarantees the memory consistency if 0 is returned.
	/// 
	/// \returns The decremented value.
	IntegralType decrement_fence0 () NIRVANA_NOEXCEPT
	{
		IntegralType ret = decrement_seq ();
		if (0 == ret)
			std::atomic_thread_fence (std::memory_order_acquire);
		return ret;
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
class RefCounter : private AtomicCounter <false>
{
	typedef AtomicCounter <false> Base;
public:
	/// The integral type of the reference counter.
	using Base::IntegralType;

	/// Initialized with 1.
	RefCounter () NIRVANA_NOEXCEPT :
		Base (1)
	{}

	/// Initialized with 1 on copy construction.
	RefCounter (const RefCounter&) NIRVANA_NOEXCEPT :
		Base (1)
	{}

	RefCounter (RefCounter&&) NIRVANA_NOEXCEPT :
		Base (1)
	{}

	/// Not changed on copy operation.
	RefCounter& operator = (const RefCounter&) NIRVANA_NOEXCEPT
	{
		return *this;
	}

	RefCounter& operator = (RefCounter&&) NIRVANA_NOEXCEPT
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
	void increment () NIRVANA_NOEXCEPT
	{
		assert (cnt_ > 0);
		Base::increment ();
	}

	/// Decrement the counter and return the decremented value.
	/// Guarantees the memory consistency if returned 0.
	/// 
	/// \returns The decremented value.
	IntegralType decrement () NIRVANA_NOEXCEPT
	{
		return Base::decrement_fence0 ();
	}
};

}
}

#endif
