//! \brief	Declares the object map class - core synchronization primitive.

#ifndef NIRVANA_CORE_OBJECTMAP_H_
#define NIRVANA_CORE_OBJECTMAP_H_

#include "RWLock.h"
#include "WaitablePtr.h"
#include <CORBA/Exception.h>
#include <unordered_map>
#include <atomic>

namespace Nirvana {
namespace Core {

class ObjectMapBase
{
public:
	struct Value
	{
		WaitablePtr ptr;
		RefCounter ref_cnt;
	};
};

template <class Key,
	class Hash = std::hash <Key>,
	class Pred = std::equal_to <Key> >
class ObjectMap :
	private std::unordered_map <Key, ObjectMapBase::Value, Hash, Pred>,
	private ObjectMapBase,
	private RWLock
{
	typedef std::unordered_map <Key, ObjectMapBase::Value, Hash, Pred> Base;
public:
	typedef Base::iterator iterator;
	
	void* get_ref (const Key& key);

	void remove_ref (iterator p);

protected:
	virtual void* create (iterator it) = 0;
	virtual void* destroy (void* p) = 0;
};

template <class Key, class Hash, class Pred>
void* ObjectMap <Key, Hash, Pred>::get_ref (const Key& key)
{
	enter_read ();
	iterator p = Base::find (key);
	if (p == Base::end ()) {
		leave_read ();
		enter_write ();
		std::pair <iterator, bool> ins;
		try {
			ins = Base::emplace (key, Value ());
		} catch (...) {
			leave_write ();
			throw;
		}
		p = ins.first;
		if (!ins.second)
			p->second.ref_cnt.increment ();
		leave_write ();

		if (ins.second) {
			// Create object
			void* obj;
			try {
				obj = create (p);
				p->second.ptr.set_object (obj);
			} catch (const CORBA::Exception& ex) {
				p->second.ptr.set_exception (ex);
				remove_ref (p);
				throw;
			} catch (...) {
				p->second.ptr.set_unknown_exception ();
				remove_ref (p);
				throw;
			}

			return obj;
		}
	} else {
		p->second.ref_cnt.increment ();
		leave_read ();
	}

	return p->second.ptr.wait ();
}

template <class Key, class Hash, class Pred>
void ObjectMap <Key, Hash, Pred>::remove_ref (iterator p)
{
	enter_read ();
	if (p->second.ref_cnt.decrement ()) {
		leave_read ();
		return;
	}
	p->second.ref_cnt.increment ();
	leave_read ();
	enter_write ();
	if (!p->second.ref_cnt.decrement ()) {
		void* obj = p->second.ptr;
		bool is_object = p->second.ptr.is_object ();
		Base::erase (p);
		leave_write ();
		if (is_object)
			destroy (obj);
		else
			delete (CORBA::Exception*)obj;
		return;
	}
	leave_write ();
}

template <class Key,
	class Obj,
	class Hash = std::hash <Key>,
	class Pred = std::equal_to <Key> >
class ObjectMapT :
	public ObjectMap <Key, Hash, Pred>
{
public:
	Obj* get_ref (const Key& key)
	{
		return (Obj*)ObjectMap <Key, Hash, Pred>::get_ref (key);
	}
};

}
}

#endif
