#ifndef NIRVANA_CORE_STRINGMANAGER_H_
#define NIRVANA_CORE_STRINGMANAGER_H_

#include "../core.h"
#include "ReferenceCounterImpl.h"
#include <CORBA/Implementation.h>
#include <algorithm>
#include <limits>

namespace Nirvana {
namespace Core {

using namespace CORBA;

class StringBase :
	public CORBA::Nirvana::ServantTraits <StringBase>,
	public CORBA::Nirvana::LifeCycleRefCnt <StringBase>,
	public CORBA::Nirvana::InterfaceImpl <StringBase, CORBA::Nirvana::DynamicServant>,
	public ReferenceCounterBase
{
public:
	static void initialize ()
	{
		heap_ = g_heap_factory->create_with_granularity (sizeof (StringBase) * 2);
	}

	static void terminate ()
	{
		CORBA::release (heap_);
	}

	static StringBase* create (ULong size, Flags memory_flags = 0);

	void _delete ()
	{
		UWord cb = sizeof (StringBase) + allocated_size_;
		this->~StringBase ();
		heap ()->release (this, cb);
	}

	static StringBase* get_object (const void* p)
	{
		if (heap ()->is_writable (p, 1))
			return (StringBase*)p - 1;
		else
			return nullptr;
	}

	const void* data () const
	{
		return this + 1;
	}

	void* data ()
	{
		return this + 1;
	}

	ULong size () const
	{
		return size_;
	}

	ULong allocated_size () const
	{
		return allocated_size_;
	}

	StringBase* get_writable_copy (ULong size);

	StringBase* get_writable_copy ()
	{
		return get_writable_copy (size_);
	}

protected:
	static Memory_ptr heap ()
	{
		return heap_;
	}

	StringBase (ULong size);

private:
	static CORBA::Nirvana::Bridge <Memory>* heap_;

	ULong size_;
	ULong allocated_size_;
};

template <class C>
class String :
	public StringBase
{
public:
	static const ULong MAX_LENGTH = std::min (std::numeric_limits <ULong>::max (), std::numeric_limits <UWord>::max () - sizeof (StringBase)) / sizeof (C) - 1;

	static ULong max_size ()
	{
		return MAX_LENGTH;
	}

	static String <C>* create (ULong length)
	{
		ULong size = (length + 1) * sizeof (C);
		return static_cast <String <C>*> (StringBase::create (size, Memory::ZERO_INIT));
	}

	static String <C>* create (ULong length, const C* pc)
	{
		ULong size = (length + 1) * sizeof (C);
		String <C>* s = static_cast <String <C>*> (StringBase::create (size));
		heap ()->copy (s->data (), const_cast <C*> (pc), size, 0);
		return s;
	}

	static String <C>* get_object (const C* p)
	{
		return static_cast <String <C>*> (StringBase::get_object (p));
	}

	static String <C>* get_writable_copy (C* p, ULong index = 0);

	String <C>* get_writable_copy (ULong size)
	{
		return static_cast <String <C>*> (StringBase::get_writable_copy ((size + 1) * sizeof (C)));
	}

	String <C>* get_writable_copy ()
	{
		return static_cast <String <C>*> (StringBase::get_writable_copy ());
	}

	C* data ()
	{
		return (C*)StringBase::data ();
	}

	const C* data () const
	{
		return (const C*)StringBase::data ();
	}

	ULong size () const
	{
		return StringBase::size () / sizeof (C) - 1;
	}

	ULong length () const
	{
		return size ();
	}

	const C* c_str () const
	{
		return data ();
	}

};

template <class C>
String <C>* String <C>::get_writable_copy (C* pc, ULong index)
{
	String <C>* s = get_object (pc);
	if (s) {
		if (index >= s->size ())
			throw BAD_PARAM ();
		s = s->get_writable_copy ();
	} else {
		// Read-only string
		ULong length = 0;
		for (const C* end = pc; *end; ++end) {
			if (length == MAX_LENGTH)
				throw BAD_PARAM ();
			++length;
		}
		if (index >= length)
			throw BAD_PARAM ();
		s = create (length, pc);
	}
	return s;
}

}
}

#endif

