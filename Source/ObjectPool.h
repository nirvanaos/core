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
class ObjectPoolT :
	private StackT <T>
{
public:
	T* get ()
	{
		T* obj = StackT <T>::pop ();
		if (!obj)
			obj = new T;
		else
			obj->activate ();
		return obj;
	}

	void release (T& obj)
	{
		StackT <T>::push (obj);
	}

	void initialize ()
	{
	}

	void terminate ()
	{
		while (T* obj = StackT <T>::pop ()) {
			delete obj;
		}
	}
};

}
}

#endif
