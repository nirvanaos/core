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
	void construct (Args ... args)
	{
		assert (!constructed_);
		new (&storage_) T (std::forward <Args> (args)...);
#ifdef _DEBUG
		constructed_ = true;
#endif
	}

	/// Destruct object in static memory.
	void destruct () NIRVANA_NOEXCEPT
	{
		assert (constructed_);
		((T&)storage_).~T ();
#ifdef _DEBUG
		constructed_ = false;
#endif
	}

	operator T& () NIRVANA_NOEXCEPT
	{
		assert (constructed_);
		return (T&)storage_;
	}

	T* operator -> () NIRVANA_NOEXCEPT
	{
		assert (constructed_);
		return (T*)&storage_;
	}

	T* operator & () NIRVANA_NOEXCEPT
	{
		assert (constructed_);
		return (T*)&storage_;
	}

private:
	typename std::aligned_storage <sizeof (T), alignof (T)>::type storage_;
#ifdef _DEBUG
	bool constructed_;
#endif
};

}
}

#endif
