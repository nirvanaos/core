#include "StringManager.h"
#include <CORBA/StringManager_s.h>
#include <limits>

namespace Nirvana {
namespace Core {

using namespace CORBA;

CORBA::Nirvana::Bridge <Memory>* StringManagerBase::heap_;

StringManagerBase::String* StringManagerBase::String::create (ULong size)
{
	String* p = (String*)heap ()->allocate (0, sizeof (String) + size, 0);
	new (p) String (size);
	return p;
}

StringManagerBase::String::String (ULong size) :
	ReferenceCounterBase (this),
	size_ (size),
	allocated_size_ ((ULong)(round_up (sizeof (String) + size_, heap ()->query (this, Memory::ALLOCATION_UNIT)) - sizeof (String)))
{}

StringManagerBase::String* StringManagerBase::String::get_writable_copy (ULong size)
{
	if ((_refcount_value () == 1) && (allocated_size_ >= size)) {
		if (size_ < size)
			heap ()->commit ((Octet*)data () + size_, size - size_);
		size_ = size;
		return this;
	}
	String* ns = (String*)heap ()->copy (nullptr, this, sizeof (String) + size, Memory::ALLOCATE | Memory::READ_WRITE);
	new (ns) String (size);
	release (this);
	return ns + 1;
}

template <class C>
class StringManager :
	public CORBA::Nirvana::ServantStatic <StringManager <C>, CORBA::Nirvana::StringManager <C> >,
	public StringManagerBase
{
public:
	static const ULong MAX_LENGTH = std::numeric_limits <ULong>::max () / sizeof (C);

	C* string_alloc (ULong length)
	{
		if (length > MAX_LENGTH)
			throw IMP_LIMIT ();
		ULong size = (length + 1) * sizeof (C);
		String* s = String::create (size);
		C* ret = (C*)(s + 1);
		ret [length] = 0;
		return ret;
	}

	C* string_dup (const C* p)
	{
		if (p) {
			String* s = String::get_object (p);
			if (s)
				s->_add_ref ();
		}
		return const_cast <C*> (p);
	}

	void string_free (C* p)
	{
		if (p) {
			String* s = String::get_object (p);
			if (s)
				s->_remove_ref ();
		}
	}

	C& at (C*& p, ULong index)
	{
		C* pc = p;
		if (!pc)
			throw BAD_PARAM ();

		String* s = String::get_object (pc);
		if (s) {
			ULong length = s->size () / sizeof (C) - 1;
			if (index >= length)
				throw BAD_PARAM ();
			s = s->get_writable_copy (s->size ());
		} else {
			// Read-only string
			ULong length = 0;
			for (const C* end = pc; *end; ++end) {
				if (length == MAX_LENGTH)
					throw BAD_PARAM ();
				++length;
			}
			ULong size = (length + 1) * sizeof (C);
			s = String::create (size);
			//heap ()->copy (s->data (), pc, size, 0);
			memcpy (s->data (), pc, size);
		}
		pc = (C*)s->data ();
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