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
#ifndef NIRVANA_CORE_OBJECTPOOL_H_
#define NIRVANA_CORE_OBJECTPOOL_H_
#pragma once

#include "Stack.h"
#include "Timer.h"

namespace Nirvana {
namespace Core {

class ObjectPoolBottom
{
public:
	void bottom () noexcept
	{
		cleanup_.store (false, std::memory_order_relaxed);
	}

	bool cleanup () noexcept
	{
		return cleanup_.exchange (true, std::memory_order_relaxed);
	}

private:
	std::atomic <bool> cleanup_;
};

/// Object pool
/// 
/// \tparam T Object type.
///           T must derive StackElem and have default constructor.
template <class T, unsigned ALIGN = core_object_align (sizeof (T))>
class ObjectPool :
	private Stack <T, ALIGN, ObjectPoolBottom>,
	private Timer
{
	typedef Stack <T, ALIGN, ObjectPoolBottom> Base;

public:
	static const TimeBase::TimeT DEFAULT_HOUSEKEEPING_INTERVAL = 10 * TimeBase::SECOND;

	/// Constructor.
	/// 
	/// \param housekeeping_interval Housekeeping interval.
	ObjectPool (const TimeBase::TimeT& housekeeping_interval = DEFAULT_HOUSEKEEPING_INTERVAL)
	{
		Timer::set (0, housekeeping_interval, housekeeping_interval);
	};

	/// Destructor.
	/// Deletes all objects from the pool.
	~ObjectPool ()
	{
		while (T* obj = Base::pop ())
			delete obj;
	}

	using ObjRef = typename std::conditional <std::is_object <Ref <T> >::value, Ref <T>, T*>::type;

	/// Tries to get object from the pool.
	/// If the pool is empty, creates a new object.
	/// 
	/// \returns object reference.
	ObjRef create ()
	{
		ObjRef ret;
		T* obj = Base::pop ();
		if (obj)
			ret = obj;
		else {
			Base::bottom ();
			create_new (ret);
		}

		assert (((uintptr_t)static_cast <T*> (ret) & Base::Ptr::ALIGN_MASK) == 0);
		return ret;
	}

	/// Release object to the pool.
	/// 
	/// \param obj Object to release.
	void release (T& obj) noexcept
	{
		Base::push (obj);
	}

private:
	static void create_new (Ref <T>& ref)
	{
		ref = Ref <T>::template create <T> ();
	}

	static void create_new (T*& ref)
	{
		ref = new T ();
	}

	void signal () noexcept override
	{
		if (Base::cleanup ())
			delete Base::pop ();
	}

};

}
}

#endif
