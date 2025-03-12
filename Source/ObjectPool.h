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

namespace Nirvana {
namespace Core {

/// Object pool
/// 
/// \tparam T Object type.
///           T must derive StackElem and have default constructor.
template <class T, unsigned ALIGN = core_object_align (sizeof (T))>
class ObjectPool : private Stack <T, ALIGN>
{
	typedef Stack <T> Base;

public:
	/// Constructor.
	/// 
	/// \param max_size Maximal object count in the pool.
	ObjectPool (unsigned max_size) :
		max_size_ (max_size),
		size_ (0)
	{};

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
		if (obj) {
			if (obj)
				--size_;
			ret = obj;
		} else
			create_new (ret);

		assert (((uintptr_t)static_cast <T*> (ret) & Base::Ptr::ALIGN_MASK) == 0);
		return ret;
	}

	/// Release object to the pool.
	/// 
	/// If pool size reached the limit, object will be deleted.
	/// 
	/// \param obj Object to release.
	void release (T& obj) noexcept
	{
		if (max_size_ < ++size_) {
			--size_;
			delete& obj;
		} else
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

private:
	const unsigned max_size_;
	std::atomic <unsigned> size_;
};

}
}

#endif
