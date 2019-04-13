// Nirvana project.
// Lock-free object pool.
#ifndef NIRVANA_CORE_OBJECTPOOL_H_
#define NIRVANA_CORE_OBJECTPOOL_H_

#include <atomic>

namespace Nirvana {
namespace Core {

/**
In the current implementation, poolable objects are never be deleted until the terminate() call.
In the future possible more complex and smart implementation.
*/
class PoolableObject
{
public:
	void activate () {}

private:
	friend class ObjectPool;
	std::atomic <PoolableObject*> next_;
};

class ObjectPool
{
public:
	void initialize ()
	{
		top_ = nullptr;
	}

	void release (PoolableObject& obj);

	PoolableObject* get ();

private:
	std::atomic <PoolableObject*> top_;
};

template <class T>
class ObjectPoolT :
	private ObjectPool
{
public:
	T* get ()
	{
		T* obj = static_cast <T*> (ObjectPool::get ());
		if (!obj)
			obj = new T;
		else
			obj->activate ();
		return obj;
	}

	void release (T& obj)
	{
		ObjectPool::release (obj);
	}

	void initialize ()
	{
		ObjectPool::initialize ();
	}

	void terminate ()
	{
		while (T* obj = static_cast <T*> (ObjectPool::get ())) {
			delete obj;
		}
	}
};

}
}

#endif
