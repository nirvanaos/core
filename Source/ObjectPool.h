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

#include "Stack.h"
#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

template <class T> class ObjectPool;

/// Poolable implementation of a core object.
/// \tparam T Object class.
///           Class T must have the members:
///           void _activate ();
///           void _deactivate (ImplPoolable <T>& obj);
/// 
/// While element is not in stack, the StackElem::next field is unused and may contain any value.
template <class T>
class ImplPoolable :
	public T,
	public StackElem
{
private:
	template <class> friend class Core_ref;

	template <class ... Args>
	ImplPoolable (Args ... args) :
		T (std::forward <Args> (args)...)
	{}

	void _add_ref ()
	{
		StackElem::ref_cnt.increment ();
	}

	void _remove_ref ();
};

/// Lock-free object pool.
/// \tparam T Poolable object class.
template <class T>
class ObjectPool :
	private Stack <ImplPoolable <T> >
{
public:
	/// Get object from the pool or create a new.
	Core_ref <T> get ()
	{
		Core_ref <ImplPoolable <T> > obj (Stack <ImplPoolable <T> >::pop ());
		if (obj)
			obj->_activate ();
		else
			obj = Core_ref <ImplPoolable <T> >::template create <ImplPoolable <T> > ();
		return obj;
	}

	/// Releas object to the pool.
	void release (ImplPoolable <T>& obj)
	{
		Stack <ImplPoolable <T> >::push (obj);
	}

	/// \brief Clean up the pool. We hardly ever call this method.
	void cleanup ()
	{
		while (T* obj = Stack <ImplPoolable <T> >::pop ()) {
			delete obj;
		}
	}
};

template <class T>
void ImplPoolable <T>::_remove_ref ()
{
	if (!StackElem::ref_cnt.decrement ())
		T::_deactivate (*this);
}

}
}

#endif
