// Nirvana project.
// Lock-free object pool.
#ifndef NIRVANA_CORE_OBJECTPOOL_H_
#define NIRVANA_CORE_OBJECTPOOL_H_

#include "Stack.h"

namespace Nirvana {
namespace Core {

class PoolableObject :
	public StackElem
{
public:
	void activate () {}
};

template <class T>
class ObjectPool :
	private Stack <T>
{
public:
	T* get ()
	{
		T* obj = Stack <T>::pop ();
		if (!obj)
			obj = new T;
		else
			obj->activate ();
		return obj;
	}

	void release (T& obj)
	{
		Stack <T>::push (obj);
	}

	void initialize ()
	{
	}

	void terminate ()
	{
		while (T* obj = Stack <T>::pop ()) {
			delete obj;
		}
	}
};

}
}

#endif
