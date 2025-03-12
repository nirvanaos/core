/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include "StaticallyAllocated.h"
#include <Port/config.h>

namespace Nirvana {
namespace Core {

class ObjectPoolCleanup
{
public:
	void bottom () noexcept
	{
		// Object pool bottom is reached, reset cleanup flag.
		cleanup_.store (false, std::memory_order_relaxed);
	}

	bool cleanup () noexcept
	{
		return cleanup_.exchange (true, std::memory_order_relaxed);
	}

private:
	std::atomic <bool> cleanup_;
};

template <class T>
class ObjectCreator
{
public:
	using ObjRef = typename std::conditional <std::is_object <Ref <T> >::value, Ref <T>, T*>::type;

	static void create (Ref <T>& ref)
	{
		ref = Ref <T>::template create <T> ();
	}

	static void create (T*& ref)
	{
		ref = new T ();
	}

	static void release (T& obj) noexcept
	{
		delete& obj;
	}
};

class NIRVANA_NOVTABLE ObjectPoolBase
{
public:
	static void housekeeping_start ()
	{
		if (pool_list_) {
			timer_.construct ();
			timer_->set (0, OBJECT_POOL_HOUSEKEEPING_PERIOD, OBJECT_POOL_HOUSEKEEPING_PERIOD);
		}
	}

	static void housekeeping_stop () noexcept
	{
		if (pool_list_)
			timer_.destruct ();
	}

protected:
	ObjectPoolBase ();

	virtual void housekeeping () noexcept = 0;

private:
	class Timer : public Core::Timer
	{
	private:
		void signal () noexcept override;
	};

private:
	ObjectPoolBase* next_;

	static ObjectPoolBase* pool_list_;
	static StaticallyAllocated <Timer> timer_;
};

/// Object pool
/// 
/// \tparam T Object type.
///           T must derive StackElem and have default constructor.
template <class T, unsigned ALIGN = core_object_align (sizeof (T))>
class ObjectPool :
	private Stack <T, ALIGN, ObjectPoolCleanup>,
	private ObjectCreator <T>,
	private ObjectPoolBase
{
	typedef Stack <T, ALIGN, ObjectPoolCleanup> Base;
	typedef ObjectCreator <T> Creator;
	typedef Creator::ObjRef ObjRef;

public:
	/// Constructor.
	/// 
	/// \param housekeeping_interval Housekeeping interval.
	ObjectPool ()
	{}

	/// Destructor.
	/// Deletes all objects from the pool.
	~ObjectPool ()
	{
		while (T* obj = Base::pop ())
			Creator::release (*obj);
	}

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
			Creator::create (ret);
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
	void housekeeping () noexcept override
	{
		if (Base::cleanup ()) {
			T* obj = Base::pop ();
			if (obj)
				Creator::release (*obj);
		}
	}

};

}
}

#endif
