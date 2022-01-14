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

#include <Nirvana/NirvanaBase.h>
#include <assert.h>
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

	operator IntegralType () const NIRVANA_NOEXCEPT
	{
		return cnt_;
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

protected:
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

	/// If we have no references we can't add a new one.
	IntegralType increment () NIRVANA_NOEXCEPT
	{
		assert (cnt_ > 0);
		return AtomicCounter::increment ();
	}
};

}
}

#endif
