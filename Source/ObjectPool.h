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
#include "TimerAsyncCall.h"
#include "Heap.h"
#include <Port/config.h>

namespace Nirvana {
namespace Core {

template <class ObjRef> class ObjectCreator;

template <class T>
class ObjectCreator <Ref <T> >
{
public:
	typedef T Object;

	static Ref <Object> create ()
	{
		return Ref <Object>::template create <Object> ();
	}

	static void release (Object* obj) noexcept
	{
		delete obj;
	}
};

template <class T>
class ObjectCreator <T*>
{
public:
	typedef T Object;

	static Object* create ()
	{
		return new Object ();
	}

	static void release (Object* obj) noexcept
	{
		delete obj;
	}
};

class NIRVANA_NOVTABLE ObjectPoolBase
{
public:
	static void housekeeping_start ()
	{
		if (pool_list_) {
			reinterpret_cast <Ref <Timer>&> (timer_) = Ref <Timer>::create <ImplDynamic <Timer> > ();
			timer_->set (0, OBJECT_POOL_SHRINK_PERIOD, OBJECT_POOL_SHRINK_PERIOD);
		}
	}

	static void housekeeping_stop () noexcept
	{
		if (pool_list_)
			reinterpret_cast <Ref <Timer>&> (timer_).~Ref <Timer> ();
	}

protected:
	ObjectPoolBase ();

	virtual void shrink () noexcept = 0;

private:
	class Timer :
		public TimerAsyncCall,
		public SharedObject
	{
	protected:
		Timer () :
			TimerAsyncCall (g_core_free_sync_context, GC_DEADLINE)
		{}

	private:
		void run (const TimeBase::TimeT&) noexcept override;
	};

private:
	ObjectPoolBase* next_;

	static ObjectPoolBase* pool_list_;
	static Timer* timer_;
};

class ObjectPoolCleanup
{
public:
	void bottom () noexcept
	{
		// Object pool bottom is reached, reset cleanup flag.
		shrink_.store (false, std::memory_order_relaxed);
	}

	bool shrink () noexcept
	{
		return shrink_.exchange (true, std::memory_order_relaxed);
	}

private:
	std::atomic <bool> shrink_;
};

template <class ObjRef>
using ObjectPoolStack = Stack <
	typename ObjectCreator <ObjRef>::Object,
	core_object_align (sizeof (typename ObjectCreator <ObjRef>::Object)),
		ObjectPoolCleanup>;

/// Object pool
/// 
/// \tparam RefType Object reference type.
///   Object must derive StackElem and have default constructor.
template <class ObjRef>
class ObjectPool :
	private ObjectPoolBase,
	private ObjectCreator <ObjRef>,
	private ObjectPoolStack <ObjRef>
{
	typedef ObjectPoolStack <ObjRef> Stack;
	typedef ObjectCreator <ObjRef> Creator;

public:
	typedef typename ObjectCreator <ObjRef>::Object Object;

	/// Constructor.
	/// 
	/// \param housekeeping_interval Housekeeping interval.
	ObjectPool ()
	{}

	/// Destructor.
	/// Deletes all objects from the pool.
	~ObjectPool ()
	{
		while (Object* obj = Stack::pop ())
			Creator::release (obj);
	}

	/// Tries to get object from the pool.
	/// If the pool is empty, creates a new object.
	/// 
	/// \returns object reference.
	ObjRef create ()
	{
		ObjRef ret;
		Object* obj = Stack::pop ();
		if (obj)
			ret = obj;
		else {
			Stack::bottom ();
			ret = Creator::create ();
		}

		assert (((uintptr_t)static_cast <Object*> (ret) & Stack::Ptr::ALIGN_MASK) == 0);
		return ret;
	}

	/// Release object to the pool.
	/// 
	/// \param obj Object to release.
	void release (Object* obj) noexcept
	{
		Stack::push (*obj);
	}

private:
	void shrink () noexcept override
	{
		if (Stack::shrink ()) {
			Object* obj = Stack::pop ();
			if (obj)
				Creator::release (obj);
		}
	}

};

}
}

#endif
