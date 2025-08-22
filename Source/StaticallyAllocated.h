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
#ifndef NIRVANA_CORE_STATICALLYALLOCATED_H_
#define NIRVANA_CORE_STATICALLYALLOCATED_H_
#pragma once

#include <Nirvana/Nirvana.h>

namespace Nirvana {
namespace Core {

/// Statically allocated object.
/// \tparam T Object type.
template <class T>
class StaticallyAllocated
{
public:
	/// Construct object in static memory.
	/// 
	/// \param ...args Arguments for constructor.
	template <class ... Args>
	void construct (Args&& ... args)
	{
		assert (!constructed_);
		new (&storage_) T (std::forward <Args> (args)...);
#ifndef NDEBUG
		constructed_ = (const T*)&storage_;
#endif
	}

	/// Destruct object in static memory.
	void destruct () noexcept
	{
		assert (constructed_);
		((T&)storage_).T::~T ();
#ifndef NDEBUG
		constructed_ = nullptr;
#endif
	}

	operator T& () noexcept
	{
		assert (constructed_);
		return (T&)storage_;
	}

	T* operator -> () noexcept
	{
		assert (constructed_);
		return (T*)&storage_;
	}

	T* operator & () noexcept
	{
		assert (constructed_);
		return (T*)&storage_;
	}

	template <class T1>
	T& operator = (T1 src)
	{
		(T&)storage_ = src;
		return (T&)storage_;
	}

	constexpr T& reference () noexcept
	{
		return (T&)storage_;
	}

private:
	typename std::aligned_storage <sizeof (T), alignof (T)>::type storage_;
#ifndef NDEBUG
	const T* constructed_;
#endif
};

}
}

#endif
