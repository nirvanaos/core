#include "StringManager.h"
#include <CORBA/StringManager_s.h>
#include <limits>

namespace Nirvana {
namespace Core {

using namespace CORBA;

CORBA::Nirvana::Bridge <Memory>* StringBase::heap_;

StringBase* StringBase::create (ULong size, Flags memory_flags)
{
	StringBase* p = (StringBase*)heap ()->allocate (0, sizeof (StringBase) + size, memory_flags);
	new (p) StringBase (size);
	return p;
}

StringBase::StringBase (ULong size) :
	ReferenceCounterBase (this),
	size_ (size),
	allocated_size_ ((ULong)(round_up (sizeof (StringBase) + size_, heap ()->query (this, Memory::ALLOCATION_UNIT)) - sizeof (StringBase)))
{}

StringBase* StringBase::get_writable_copy (ULong size)
{
	if ((_refcount_value () == 1) && (allocated_size_ >= size)) {
		if (size_ < size)
			heap ()->commit ((Octet*)data () + size_, size - size_);
		size_ = size;
		return this;
	}
	StringBase* ns = (StringBase*)heap ()->copy (nullptr, this, sizeof (StringBase) + size, Memory::ALLOCATE | Memory::READ_WRITE);
	new (ns) StringBase (size);
	release (this);
	return ns;
}

template <class C>
class StringManager :
	public CORBA::Nirvana::ServantStatic <StringManager <C>, CORBA::Nirvana::StringManager <C> >
{
public:
	C* string_alloc (ULong length)
	{
		if (length > String <C>::max_size ())
			throw IMP_LIMIT ();
		String <C>* s = String <C>::create (length);
		return s->data ();
	}

	C* string_dup (const C* p)
	{
		if (p) {
			String <C>* s = String <C>::get_object (p);
			if (s)
				s->_add_ref ();
		}
		return const_cast <C*> (p);
	}

	void string_free (C* p)
	{
		if (p) {
			String <C>* s = String <C>::get_object (p);
			if (s)
				s->_remove_ref ();
		}
	}

	C& at (C*& p, ULong index)
	{
		C* pc = p;
		if (!pc)
			throw BAD_PARAM ();

		String <C>* s = String <C>::get_writable_copy (pc, index);
		pc = s->data ();
		p = pc;
		return pc [index];
	}
};

}
}

namespace CORBA {
namespace Nirvana {

Bridge <StringManager <Char> >* const StringManager <Char>::singleton_ = STATIC_BRIDGE (::Nirvana::Core::StringManager <Char>, StringManager <Char>);
Bridge <StringManager <WChar> >* const StringManager <WChar>::singleton_ = STATIC_BRIDGE (::Nirvana::Core::StringManager <WChar>, StringManager <WChar>);

}
}