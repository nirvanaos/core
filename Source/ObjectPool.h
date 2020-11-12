// Nirvana project.
// Lock-free object pool.
#ifndef NIRVANA_CORE_OBJECTPOOL_H_
#define NIRVANA_CORE_OBJECTPOOL_H_

#include "Stack.h"
#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

template <class T> class ObjectPool;

/// Poolable implementation of a core object.
/// \tparam T object class.
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
	template <class> friend class Core_var;

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

template <class T>
class ObjectPool :
	private Stack <ImplPoolable <T> >
{
public:
	Core_var <T> get ()
	{
		Core_var <ImplPoolable <T> > obj (Stack <ImplPoolable <T> >::pop ());
		if (obj)
			obj->_activate ();
		else
			obj = Core_var <ImplPoolable <T> >::template create <ImplPoolable <T> > ();
		return obj;
	}

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
