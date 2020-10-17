// Nirvana project.
// Lock-free object pool.
#ifndef NIRVANA_CORE_OBJECTPOOL_H_
#define NIRVANA_CORE_OBJECTPOOL_H_

#include "Stack.h"

namespace Nirvana {
namespace Core {

template <class T>
class ObjectPool :
	private Stack <T>
{
public:
	T* get ()
	{
		T* obj = Stack <T>::pop ();
		if (obj)
			obj->_activate ();
		else
			obj = new T;
		return obj;
	}

	void release (T& obj)
	{
		obj._deactivate ();
		Stack <T>::push (obj);
	}

	/// \brief Clean up the pool. We hardly ever call this method.
	void cleanup ()
	{
		while (T* obj = Stack <T>::pop ()) {
			delete obj;
		}
	}
};

/// Poolable implementation of a core object.
/// \tparam T object class.
/// \tparam I interfaces.
template <class T, class ... I>
class ImplPoolable :
	public CoreObject, // Allocate from core heap.
	public StackElem,
	public I...
{
public:
	static T* get ()
	{
		return pool_.get ();
	}

	void _activate ()
	{
		::new (&static_cast <StackElem&> (*this).ref_cnt) RefCounter (); // Initialize reference counter with 1
	}

	void _add_ref ()
	{
		StackElem::ref_cnt.increment ();
	}

	void _remove_ref ()
	{
		if (!StackElem::ref_cnt.decrement ())
			pool_.release (static_cast <T&> (*this));
	}

	static ObjectPool <T> pool_;
};

template <class T, class ... I>
ObjectPool <T> ImplPoolable <T, I...>::pool_;

}
}

#endif
