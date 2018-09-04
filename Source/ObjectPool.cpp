// Nirvana project.
// Lock-free object pool.

#include "ObjectPool.h"

namespace Nirvana {
namespace Core {

void ObjectPool::release (PoolableObject& obj)
{
	PoolableObject* next = top_.load ();
	do {
		obj.next_ = next;
	} while (!top_.compare_exchange_weak (next, &obj));
}

PoolableObject* ObjectPool::get ()
{
	PoolableObject* first = top_.load ();
	while (first) {
		// Note that object is never be deleted, so we can be sure that pointer is valid.
		PoolableObject* next = first->next_;
		if (top_.compare_exchange_weak (first, next))
			break;
	}
	return first;
}

}
}
